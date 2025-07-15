#!/usr/bin/python3
# SPDX-FileCopyrightText: 2025 Tim Cocks for Adafruit Industries
#
# SPDX-License-Identifier: MIT
"""
Display a simple test pattern of 3 shapes on three 64x64 matrix panels
using Active3 compatible connections.

Run like this:

$ python triple_matrix_active3_simpletest.py

"""

import numpy as np
from PIL import Image, ImageDraw

import adafruit_blinka_raspberry_pi5_piomatter as piomatter
from adafruit_blinka_raspberry_pi5_piomatter.pixelmappers import simple_multilane_mapper

width = 64
n_lanes = 6
n_addr_lines = 5
height = n_lanes << n_addr_lines
pen_radius = 1

canvas = Image.new('RGB', (width, height), (0, 0, 0))
draw = ImageDraw.Draw(canvas)

pixelmap = simple_multilane_mapper(width, height, n_addr_lines, n_lanes)
geometry = piomatter.Geometry(width=width, height=height, n_addr_lines=n_addr_lines, n_planes=10, n_temporal_planes=4, map=pixelmap, n_lanes=n_lanes)
framebuffer = np.asarray(canvas) + 0  # Make a mutable copy
matrix = piomatter.PioMatter(colorspace=piomatter.Colorspace.RGB888Packed,
                             pinout=piomatter.Pinout.Active3BGR,
                             framebuffer=framebuffer,
                             geometry=geometry)


draw.rectangle((8, 8, width-8, width-8), fill=0x008800)
draw.circle((32, 64+32), 22, fill=0x880000)
draw.polygon([(32, 136), (54, 180), (10, 180)], fill=0x000088)

framebuffer[:] = np.asarray(canvas)
matrix.show()

input("Press enter to exit")
