from ._piomatter import (
    AdafruitMatrixBonnetRGB565,
    AdafruitMatrixBonnetRGB888,
    AdafruitMatrixBonnetRGB888Packed,
    Colorspace,
    Geometry,
    Orientation,
    Pinout,
    PioMatter,
)


def make_pixelmap_multilane(width, height, n_addr_lines, n_lanes):
    calc_height = n_lanes << n_addr_lines
    if height != calc_height:
        raise RuntimeError(f"Calculated height {calc_height} does not match requested height {height}")
    n_addr = 1 << n_addr_lines

    m = []
    for addr in range(n_addr):
        for x in range(width):
            for lane in range(n_lanes):
                y = addr + lane * n_addr
                m.append(x + width * y)
    print(m)
    return m

__all__ = [
    'AdafruitMatrixBonnetRGB565',
    'AdafruitMatrixBonnetRGB888',
    'AdafruitMatrixBonnetRGB888Packed',
    'Colorspace',
    'Geometry',
    'Orientation',
    'Pinout',
    'PioMatter',
    'make_pixelmap_multilane',
]
