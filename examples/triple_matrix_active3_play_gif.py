#!/usr/bin/python3
"""
Display an animated gif across 3 matrices using the triple matrix with 3x 64x64 matrices

Run like this:

$ python triple_matrix_active3_play_gif.py

The animated gif is played repeatedly until interrupted with ctrl-c.
"""

import time

import numpy as np
import PIL.Image as Image

import adafruit_blinka_raspberry_pi5_piomatter as piomatter
from adafruit_blinka_raspberry_pi5_piomatter.pixelmappers import simple_multilane_mapper

width = 64
n_lanes = 6
n_addr_lines = 5
height = n_lanes << n_addr_lines
gif_file = "nyan.gif"

canvas = Image.new('RGB', (width, height), (0, 0, 0))
pixelmap = simple_multilane_mapper(width, height, n_addr_lines, n_lanes)
geometry = piomatter.Geometry(width=width, height=height,
                              n_addr_lines=n_addr_lines, n_planes=10,
                              n_temporal_planes=4, map=pixelmap, n_lanes=n_lanes)
framebuffer = np.asarray(canvas) + 0  # Make a mutable copy
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.Active3BGR,
                             framebuffer=framebuffer,
                             geometry=geometry)

with Image.open(gif_file) as img:
    print(f"frames: {img.n_frames}")
    while True:
        for i in range(img.n_frames):
            img.seek(i)
            img.rotate(-90)
            canvas.paste(
                img.rotate(90, expand=True)
                .resize((64, 168), resample=Image.NEAREST), (0,192//2 - 168//2))
            framebuffer[:] = np.asarray(canvas)
            matrix.show()
            time.sleep(0.1)
