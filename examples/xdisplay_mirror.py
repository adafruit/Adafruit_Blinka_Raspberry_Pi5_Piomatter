#!/usr/bin/python3
"""
Display a (possibly scaled) X display to a matrix

The display runs until this script exits.

The display doesn't get a keyboard or mouse, so you have to use a program that
will get its input in some other way, such as from a gamepad.

For help with commandline arguments, run `python xdisplay_mirror.py --help`

This example command will mirror the entire display scaled onto a 2x2 grid of 64px panels, total matrix size 128x128.

    $ python xdisplay_mirror.py --pinout AdafruitMatrixHatBGR --width 128 --height 128 --serpentine --num-address-lines 5 --num-planes 8

This example command will mirror a 128x128 pixel square from the top left of the display at real size on the same matrix panels

    $  python xdisplay_mirror.py --pinout AdafruitMatrixHatBGR --width 128 --height 128 --serpentine --num-address-lines 5 --num-planes 8 --mirror-region 0,0,128,128
"""

import click
import numpy as np
from PIL import Image, ImageGrab

import adafruit_blinka_raspberry_pi5_piomatter as piomatter
import adafruit_blinka_raspberry_pi5_piomatter.click as piomatter_click
from adafruit_blinka_raspberry_pi5_piomatter.pixelmappers import simple_multilane_mapper

RESAMPLE_MAP = {
    "nearest": Image.NEAREST,
    "bilinear": Image.BILINEAR,
    "lanczos": Image.LANCZOS,
    "bicubic": Image.BICUBIC
}


@click.command
@click.option("--mirror-region", help="Region of X display to mirror. Comma seperated x,y,w,h. "
                                      "Default will mirror entire display.", default="")
@click.option("--x-display", help="The X display to mirror. Default is :0", default=":0")
@click.option("--brightness", help="The brightness factor of the image output to the matrix",
              default=1.0, type=click.FloatRange(min=0.1, max=1.0))
@click.option("--resample-method", type=click.Choice(RESAMPLE_MAP), default="nearest",
              help="The resample method for PIL to use when resizing the screen image. Default is nearest")
@piomatter_click.standard_options(n_lanes=2, n_temporal_planes=0)
def main(width, height, serpentine, rotation, pinout, n_planes,
         n_temporal_planes, n_addr_lines, n_lanes, mirror_region, x_display, resample_method, brightness):

    if n_lanes != 2:
        pixelmap = simple_multilane_mapper(width, height, n_addr_lines, n_lanes)
        geometry = piomatter.Geometry(width=width, height=height, n_planes=n_planes, n_addr_lines=n_addr_lines,
                                      n_temporal_planes=n_temporal_planes, n_lanes=n_lanes, map=pixelmap)
    else:
        geometry = piomatter.Geometry(width=width, height=height, n_planes=n_planes, n_addr_lines=n_addr_lines,
                                      n_temporal_planes=n_temporal_planes, rotation=rotation, serpentine=serpentine)

    framebuffer = np.zeros(shape=(geometry.height, geometry.width, 3), dtype=np.uint8)
    matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed, pinout=pinout, framebuffer=framebuffer,
                                 geometry=geometry)
    matrix.brightness = brightness

    if mirror_region:
        mirror_region = tuple(int(_) for _ in mirror_region.split(','))
    else:
        mirror_region = None
        size_measure = ImageGrab.grab(xdisplay=":0")
        print(f"Mirroring full display: {size_measure.width}, {size_measure.height}")

    while True:
        img = ImageGrab.grab(xdisplay=x_display)
        if mirror_region is not None:
            img = img.crop((mirror_region[0], mirror_region[1],    # left,top
                            mirror_region[0] + mirror_region[2],   # right
                            mirror_region[1] + mirror_region[3]))  # bottom
        img = img.resize((width, height), RESAMPLE_MAP[resample_method])

        framebuffer[:, :] = np.array(img)
        matrix.show()

if __name__ == '__main__':
    main()
