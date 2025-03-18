#!/usr/bin/python3
"""
Display a (possibly scaled) X session to a matrix

The display runs until the graphical program exits.

The display doesn't get a keyboard or mouse, so you have to use a program that
will get its input in some other way, such as from a gamepad.

For help with commandline arguments, run `python virtualdisplay.py --help`

This needs additional software to be installed (besides a graphical program to run). At a minimum you have to
install a virtual display server program (xvfb) and the pyvirtualdisplay importable Python module:

    $ sudo apt install -y xvfb
    $ pip install pyvirtualdisplay

Here's an example for running an emulator using a rom stored in "/tmp/snesrom.smc" on a virtual 128x128 panel made from 4 64x64 panels:

    $ python virtualdisplay.py --pinout AdafruitMatrixHatBGR  --scale 2 --backend xvfb  --width 128 --height 128  --serpentine --num-address-lines 5 --num-planes 4  -- mednafen -snes.xscalefs 1 -snes.yscalefs 1 -snes.xres 128 -video.fs 1 -video.driver softfb  /tmp/snesrom.smc
"""

import shlex
from subprocess import Popen

import click
import numpy as np
from PIL import ImageEnhance
from pyvirtualdisplay.smartdisplay import SmartDisplay

import adafruit_blinka_raspberry_pi5_piomatter as piomatter
import adafruit_blinka_raspberry_pi5_piomatter.click as piomatter_click


@click.command
@click.option("--scale", type=float, help="The scale factor, larger numbers mean more virtual pixels",  default=1)
@click.option("--backend", help="The pyvirtualdisplay backend to use",  default="xvfb")
@click.option("--extra-args", help="Extra arguments to pass to the backend server",  default="")
@click.option("--rfbport", help="The port number for the --backend xvnc",  default=None, type=int)
@click.option("--brightness", help="The brightness factor of the image output to the matrix",
              default=1.0, type=click.FloatRange(min=0.1, max=1.0))
@click.option("--use-xauth/--no-use-xauth", help="If a Xauthority file should be created",  default=False)
@piomatter_click.standard_options
@piomatter_click.make_matrix(colorspace=piomatter.Colorspace.RGB888Packed)
@click.argument("command", nargs=-1)
def main(matrix, framebuffer, scale, backend, use_xauth, extra_args, rfbport, brightness, command):
    kwargs = {}
    if backend == "xvnc":
        kwargs['rfbport'] = rfbport
    if extra_args:
        kwargs['extra_args'] = shlex.split(extra_args)
    print("xauth", use_xauth)

    width = matrix.width
    height = matrix.height

    with SmartDisplay(backend=backend, use_xauth=use_xauth, size=(round(width*scale),round(height*scale)), manage_global_env=False, **kwargs) as disp, Popen(command, env=disp.env()) as proc:
            while proc.poll() is None:
                img = disp.grab(autocrop=False)

                if img is None:
                    continue
                if brightness != 1.0:
                    darkener = ImageEnhance.Brightness(img)
                    img = darkener.enhance(brightness)
                img = img.resize((width, height))
                framebuffer[:, :] = np.array(img)
                matrix.show()
if __name__ == '__main__':
    main()
