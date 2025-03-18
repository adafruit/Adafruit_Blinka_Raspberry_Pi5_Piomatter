# SPDX-FileCopyrightText: 2025 Jeff Epler for Adafruit Industries
# SPDX-License-Identifier: Unlicense

"""A helper for parsing piomatter settings on the commandline"""
from collections.abc import Callable
from typing import Any

import click
import numpy as np

import adafruit_blinka_raspberry_pi5_piomatter as piomatter

from .pixelmappers import simple_multilane_mapper


def make_matrix(*, colorspace):
    print("make_matrix", colorspace)
    def do_make_matrix(f):
        print("do_make_matrix", f)
        def wrapper(width, height, serpentine, rotation, pinout, n_planes, n_temporal_planes, n_addr_lines, n_lanes, **click_args):
            if colorspace == piomatter.Colorspace.RGB565:
                framebuffer = np.zeros((height, width), dtype=np.uint16)
            elif colorspace == piomatter.Colorspace.RGB888:
                framebuffer = np.zeros((height, width), dtype=np.uint32)
            elif colorspace == piomatter.Colorspace.RGB888Packed:
                framebuffer = np.zeros((height, width, 3), dtype=np.uint8)
            else:
                raise ValueError(f"Unsupported colorspace {colorspace!r}")

            if n_lanes != 2:
                if serpentine:
                    raise ValueError("Serpentine is only avaialble with 2 lanes")
                pixelmap = simple_multilane_mapper(width, height, n_addr_lines, n_lanes)
                geometry = piomatter.Geometry(width=width, height=height, n_planes=n_planes, n_addr_lines=n_addr_lines, n_temporal_planes=n_temporal_planes, n_lanes=n_lanes, map=pixelmap)
            else:
                geometry = piomatter.Geometry(width=width, height=height, n_planes=n_planes, n_addr_lines=n_addr_lines, n_temporal_planes=n_temporal_planes, rotation=rotation)
            matrix = piomatter.PioMatter(colorspace=colorspace, pinout=pinout, framebuffer=framebuffer, geometry=geometry)

            return f(matrix=matrix, framebuffer=framebuffer, **click_args)
        return wrapper
    return do_make_matrix

class _PybindEnumChoice(click.Choice):
    def __init__(self, enum, case_sensitive=False):
        self.enum = enum
        choices = [k for k, v in enum.__dict__.items() if isinstance(v, enum)]
        super().__init__(choices, case_sensitive)

    def convert(
        self, value: Any, param: click.Parameter | None, ctx: click.Context | None
    ) -> Any:
        if isinstance(value, self.enum):
            return value

        value = super().convert(value, param, ctx)
        r = getattr(self.enum, value)
        return r

def _validate_temporal_planes(ctx, param, value):
    if value not in (0, 1, 2, 3, 4, 5):
        raise click.BadParameter("must be from 0 to 5 (0 and 1 both disable temporal planes)")
    return value

def standard_options(
    f: click.decorators.FC | None = None,
    *,
    width=64,
    height=32,
    serpentine=True,
    rotation=piomatter.Orientation.Normal,
    pinout=piomatter.Pinout.AdafruitMatrixBonnet,
    n_planes=10,
    n_temporal_planes=0,
    n_addr_lines=4,
    n_lanes=2,
) -> Callable[[], None]:
    """Add standard commandline flags, with the defaults given

    Use like a click decorator:

    .. code-block:: python

        @click.command
        @piomatter_click.standard_options()
        def my_awesome_code(width, height, ...):
            ...

    If a kwarg to this function is None, then the corresponding commandline
    option is not added at all. For example, if you don't want to offer the
    ``--colorspace`` argument, write ``piomatter_click(..., colorspace=None)``."""
    def wrapper(f: click.decorators.FC):
        if width is not None:
            f = click.option("--width", default=width, help="The panel width in pixels")(f)
        if height is not None:
            f = click.option("--height", default=height, help="The panel height in pixels")(f)
        if serpentine is not None:
            f = click.option("--serpentine/--no-serpentine", default=serpentine, help="The organization of multiple panels")(f)
        if pinout is not None:
            f = click.option(
                "--pinout",
                default=pinout,
                type=_PybindEnumChoice(piomatter.Pinout),
                help="The details of the electrical connection to the panels"
            )(f)
        if rotation is not None:
            f = click.option(
                "--orientation",
                "rotation",
                default=rotation,
                type=_PybindEnumChoice(piomatter.Orientation),
                help="The overall orientation (rotation) of the panels"
            )(f)
        if n_planes is not None:
            f = click.option("--num-planes", "n_planes", default=n_planes, help="The number of bit planes (color depth). Lower values can improve refresh rate in frames per second")(f)
        if n_temporal_planes is not None:
            f = click.option("--num-temporal-planes", "n_temporal_planes", default=n_temporal_planes, callback=_validate_temporal_planes, help="The number of temporal bit-planes. May be 0, 2, or 4. Nonzero values improve frame rate but can cause some shimmer")(f)
        if n_addr_lines is not None:
            f = click.option("--num-address-lines", "n_addr_lines", default=n_addr_lines, help="The number of address lines used by the panels")(f)
        if n_lanes is not None:
            f = click.option("--num-lanes", "n_lanes", default=n_lanes, help="The number of lanes used by the panels. One 16-pin connector has two lanes (6 RGB pins)")(f)
        return f
    if f is None:
        return wrapper
    return wrapper(f)
