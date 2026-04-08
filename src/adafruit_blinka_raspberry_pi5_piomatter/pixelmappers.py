"""Functions to define the layout of complex setups, particularly multi-connector matrices"""

def simple_multilane_mapper(width, height, n_addr_lines, n_lanes):
    """A simple mapper for 4+ pixel lanes

    A framebuffer (width × height) is mapped onto a matrix where the lanes are stacked
    top-to-bottom. Panels within a lane may be cascaded left-to-right.

    Rotation is not supported, and neither are more complicated arrangements of panels
    within a single chain (no support for serpentine or stacked panels within a segment)

    .. code-block::

        0 -> [panel] -> [panel]
        1 -> [panel] -> [panel]
        2 -> [panel] -> [panel]
    """

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
    return m


def stripe_multiplex_mapper(width, height):
    """A mapper for panels with stripe multiplexing (e.g. 64x64 outdoor panels)

    Some panels interleave top/bottom row stripes across a double-width shift register.
    This mapper handles panels that use 128-pixel shifts with 5 address lines, such as the
    Adafruit 4732 (64x64 3mm pitch).

    Returns ``(framebuffer_shape, geometry_kwargs)`` where ``framebuffer_shape`` is the
    shape to use for the numpy framebuffer array and ``geometry_kwargs`` are the keyword
    arguments to pass to ``Geometry``.

    Usage::

        from adafruit_blinka_raspberry_pi5_piomatter.pixelmappers import stripe_multiplex_mapper

        fb_shape, geo_kwargs = stripe_multiplex_mapper(64, 64)
        framebuffer = np.zeros(shape=fb_shape, dtype=np.uint8)
        geometry = piomatter.Geometry(**geo_kwargs)

        # Draw into framebuffer columns 0..width-1 as normal
        framebuffer[:32, :32] = [255, 0, 0]  # top-left red
    """

    n_addr_lines = 5
    n_addr = 1 << n_addr_lines
    virt_w = width * 2
    virt_h = height
    pixels_across = virt_w * virt_h // (2 << n_addr_lines)
    half_addr = n_addr // 2

    m = []
    for addr in range(n_addr):
        for shift_x in range(pixels_across):
            for lane in range(2):
                eff_addr = addr % half_addr
                matrix_y = eff_addr + lane * half_addr
                if shift_x >= width:
                    is_top = True
                    vis_x = shift_x - width
                else:
                    is_top = False
                    vis_x = shift_x
                base = (matrix_y // half_addr) * (height // 2)
                offset = matrix_y % half_addr
                if is_top:
                    vis_y = base + offset
                else:
                    vis_y = base + offset + half_addr
                m.append(vis_x + virt_w * vis_y)

    framebuffer_shape = (virt_h, virt_w, 3)
    geometry_kwargs = {
        "width": virt_w,
        "height": virt_h,
        "n_addr_lines": n_addr_lines,
        "map": m,
        "n_lanes": 2,
    }
    return framebuffer_shape, geometry_kwargs
