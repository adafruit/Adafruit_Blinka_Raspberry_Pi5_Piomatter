#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

#include "piomatter/piomatter.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

namespace {
struct PyPiomatter {
    PyPiomatter(py::buffer buffer,
                std::unique_ptr<piomatter::piomatter_base> &&matter)
        : buffer{buffer}, matter{std::move(matter)} {}
    py::buffer buffer;
    std::unique_ptr<piomatter::piomatter_base> matter;

    void show() { matter->show(); }
    double fps() const { return matter->fps; }
};

template <typename pinout, typename colorspace>
std::unique_ptr<PyPiomatter>
make_piomatter_pc(py::buffer buffer,
                  const piomatter::matrix_geometry &geometry) {
    using cls = piomatter::piomatter<pinout, colorspace>;
    using data_type = colorspace::data_type;

    const auto n_pixels = geometry.width * geometry.height;
    const auto data_size_in_bytes = colorspace::data_size_in_bytes(n_pixels);
    const py::buffer_info info = buffer.request();
    const size_t buffer_size_in_bytes = info.size * info.itemsize;

    if (geometry.n_lanes * 3 > std::size(pinout::PIN_RGB)) {
        throw std::runtime_error(
            py::str("Geometry lane count {} exceeds the pinout with {} rgb "
                    "pins ({} lanes)")
                .attr("format")(geometry.n_lanes, std::size(pinout::PIN_RGB),
                                std::size(pinout::PIN_RGB) / 3)
                .template cast<std::string>());
    }
    if (buffer_size_in_bytes != data_size_in_bytes) {
        throw std::runtime_error(
            py::str("Framebuffer size must be {} bytes ({} elements of {} "
                    "bytes each), got a buffer of {} bytes")
                .attr("format")(data_size_in_bytes, n_pixels,
                                colorspace::data_size_in_bytes(1),
                                buffer_size_in_bytes)
                .template cast<std::string>());
    }

    std::span<data_type> framebuffer(reinterpret_cast<data_type *>(info.ptr),
                                     data_size_in_bytes / sizeof(data_type));
    return std::make_unique<PyPiomatter>(
        buffer, std::move(std::make_unique<cls>(framebuffer, geometry)));
}

enum Colorspace { RGB565, RGB888, RGB888Packed };

enum Pinout {
    AdafruitMatrixBonnet,
    AdafruitMatrixBonnetBGR,
    Active3,
    Active3BGR,
};

template <class pinout>
std::unique_ptr<PyPiomatter>
make_piomatter_p(Colorspace c, py::buffer buffer,
                 const piomatter::matrix_geometry &geometry) {
    switch (c) {
    case RGB565:
        return make_piomatter_pc<pinout, piomatter::colorspace_rgb565>(
            buffer, geometry);
    case RGB888:
        return make_piomatter_pc<pinout, piomatter::colorspace_rgb888>(
            buffer, geometry);
    case RGB888Packed:
        return make_piomatter_pc<pinout, piomatter::colorspace_rgb888_packed>(
            buffer, geometry);
    }
    throw std::runtime_error(py::str("Invalid colorspace {!r}")
                                 .attr("format")(c)
                                 .template cast<std::string>());
}

std::unique_ptr<PyPiomatter>
make_piomatter(Colorspace c, Pinout p, py::buffer buffer,
               const piomatter::matrix_geometry &geometry) {
    switch (p) {
    case AdafruitMatrixBonnet:
        return make_piomatter_p<piomatter::adafruit_matrix_bonnet_pinout>(
            c, buffer, geometry);
    case AdafruitMatrixBonnetBGR:
        return make_piomatter_p<piomatter::adafruit_matrix_bonnet_pinout_bgr>(
            c, buffer, geometry);
    case Active3:
        return make_piomatter_p<piomatter::active3_pinout>(c, buffer, geometry);
    case Active3BGR:
        return make_piomatter_p<piomatter::active3_pinout_bgr>(c, buffer,
                                                               geometry);
    }
    throw std::runtime_error(py::str("Invalid pinout {!r}")
                                 .attr("format")(p)
                                 .template cast<std::string>());
}
} // namespace

PYBIND11_MODULE(_piomatter, m) {
    py::options options;
    options.enable_enum_members_docstring();
    options.enable_function_signatures();
    options.enable_user_defined_docstrings();

    m.doc() = R"pbdoc(
        HUB75 matrix driver for Raspberry Pi 5 using PIO
        ------------------------------------------------

        .. currentmodule:: adafruit_blinka_raspberry_pi5_piomatter

        .. autosummary::
           :toctree: _generate

           Orientation
           Pinout
           Colorspace
           Geometry
           PioMatter
           AdafruitMatrixBonnetRGB888
           AdafruitMatrixBonnetRGB888Packed
    )pbdoc";

    py::enum_<piomatter::orientation>(
        m, "Orientation", "Describe the orientation of a set of panels")
        .value("Normal", piomatter::orientation::normal, "Normal orientation")
        .value("R180", piomatter::orientation::r180, "Rotated 180 degrees")
        .value("CCW", piomatter::orientation::ccw,
               "Rotated 90 degress counterclockwise")
        .value("CW", piomatter::orientation::cw,
               "Rotated 90 degress clockwise");

    py::enum_<Pinout>(
        m, "Pinout", "Describes the pins used for the connection to the matrix")
        .value("AdafruitMatrixBonnet", Pinout::AdafruitMatrixBonnet,
               "Adafruit Matrix Bonnet or Matrix Hat")
        .value("AdafruitMatrixBonnetBGR", Pinout::AdafruitMatrixBonnetBGR,
               "Adafruit Matrix Bonnet or Matrix Hat with BGR color order")
        .value("AdafruitMatrixHat", Pinout::AdafruitMatrixBonnet,
               "Adafruit Matrix Bonnet or Matrix Hat")
        .value("AdafruitMatrixHatBGR", Pinout::AdafruitMatrixBonnetBGR,
               "Adafruit Matrix Bonnet or Matrix Hat with BGR color order")
        .value("Active3", Pinout::Active3, "Active-3 or compatible board")
        .value("Active3BGR", Pinout::Active3BGR,
               "Active-3 or compatible board with BGR color order");

    py::enum_<Colorspace>(
        m, "Colorspace",
        "Describes the organization of the graphics data in memory")
        .value("RGB888Packed", Colorspace::RGB888Packed,
               "3 bytes per pixel in RGB order")
        .value("RGB888", Colorspace::RGB888, "4 bytes per pixel in RGB order")
        .value("RGB565", Colorspace::RGB565, "2 bytes per pixel in RGB order");

    py::class_<piomatter::matrix_geometry>(m, "Geometry", R"pbdoc(
Describe the geometry of a set of panels

``width`` and ``height`` give the panel resolution in pixels.

``n_addr_lines`` gives the number of connected address lines.

The number of pixels in the shift register is automatically computed from these values.

``n_planes`` controls the color depth of the panel. This is separate from the framebuffer
layout. Decreasing ``n_planes`` can increase FPS at the cost of reduced color fidelity.
The default, 10, is the maximum value.

``n_temporal_planes`` controls temporal dithering of the panel. The acceptable values
are 0 (the default), 2, and 4. A higher setting can increase FPS at the cost of
slightly increasing the variation of brightness across subsequent frames.

For simple panels with just 1 connector (2 color lanes), the following constructor arguments are available:

``serpentine`` controls the arrangement of multiple panels when they are stacked in rows.
If it is `True`, then each row goes in the opposite direction of the previous row.
If this is specified, ``n_lanes`` cannot be, and 2 lanes are always used.

``rotation`` controls the orientation of the panel(s). Must be one of the ``Orientation``
constants. Default is ``Orientation.Normal``.

For panels with more than 2 lanes, or using a custom pixel mapping, the following constructor arguments are available:

``n_lanes`` controls how many color lanes are used. A single 16-pin HUB75 connector has 2 color lanes.
If 2 or 3 connectors are used, then there are 4 or 6 lanes.

``map`` is a Python list of integers giving the framebuffer pixel indices for each matrix pixel.
)pbdoc")
        .def(py::init([](size_t width, size_t height, size_t n_addr_lines,
                         bool serpentine, piomatter::orientation rotation,
                         size_t n_planes, size_t n_temporal_planes) {
                 size_t n_lines = 2 << n_addr_lines;
                 size_t pixels_across = width * height / n_lines;
                 size_t odd = (width * height) % n_lines;
                 if (odd) {
                     throw std::runtime_error(
                         py::str(
                             "Total pixel count {} must be a multiple of {}, "
                             "the number of distinct row addresses for {}")
                             .attr("format")(width * height, n_lines,
                                             n_addr_lines)
                             .cast<std::string>());
                 }
                 switch (rotation) {
                 case piomatter::orientation::normal:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes,
                         n_temporal_planes, width, height, serpentine,
                         piomatter::orientation_normal);

                 case piomatter::orientation::r180:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes,
                         n_temporal_planes, width, height, serpentine,
                         piomatter::orientation_r180);

                 case piomatter::orientation::ccw:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes,
                         n_temporal_planes, width, height, serpentine,
                         piomatter::orientation_ccw);

                 case piomatter::orientation::cw:
                     return piomatter::matrix_geometry(
                         pixels_across, n_addr_lines, n_planes,
                         n_temporal_planes, width, height, serpentine,
                         piomatter::orientation_cw);
                 }
                 throw std::runtime_error("invalid rotation");
             }),
             py::arg("width"), py::arg("height"), py::arg("n_addr_lines"),
             py::arg("serpentine") = true,
             py::arg("rotation") = piomatter::orientation::normal,
             py::arg("n_planes") = 10u, py::arg("n_temporal_planes") = 2)
        .def(py::init([](size_t width, size_t height, size_t n_addr_lines,
                         piomatter::matrix_map map, size_t n_planes,
                         size_t n_temporal_planes, size_t n_lanes) {
                 size_t n_lines = n_lanes << n_addr_lines;
                 size_t pixels_across = width * height / n_lines;
                 for (auto el : map) {
                     if ((size_t)el >= width * height) {
                         throw std::out_of_range("Map element out of range");
                     }
                 }
                 return piomatter::matrix_geometry(pixels_across, n_addr_lines,
                                                   n_planes, n_temporal_planes,
                                                   width, height, map, n_lanes);
             }),
             py::arg("width"), py::arg("height"), py::arg("n_addr_lines"),
             py::arg("map"), py::arg("n_planes") = 10u,
             py::arg("n_temporal_planes") = 0u, py::arg("n_lanes") = 2)
        .def_readonly("width", &piomatter::matrix_geometry::width)
        .def_readonly("height", &piomatter::matrix_geometry::height);

    py::class_<PyPiomatter>(m, "PioMatter", R"pbdoc(
HUB75 matrix driver for Raspberry Pi 5 using PIO

``colorspace`` controls the colorspace that will be used for data to be displayed.
It must be one of the ``Colorspace`` constants. Which to use depends on what data
your displaying and how it is processed before copying into the framebuffer.

``pinout`` defines which pins the panels are wired to. Different pinouts can
support different hardware breakouts and panels with different color order. The
value must be one of the ``Pinout`` constants.

``framebuffer`` a numpy array that holds pixel data in the appropriate colorspace.

``geometry`` controls the size and shape of the panel. The value must be a ``Geometry``
instance.
)pbdoc")
        .def(py::init(&make_piomatter), py::arg("colorspace"),
             py::arg("pinout"), py::arg("framebuffer"), py::arg("geometry"))
        .def("show", &PyPiomatter::show, R"pbdoc(
Update the displayed image

After modifying the content of the framebuffer, call this method to
update the data actually displayed on the panel. Internally, the
data is triple-buffered to prevent tearing.
)pbdoc")
        .def_property_readonly("fps", &PyPiomatter::fps, R"pbdoc(
The approximate number of matrix refreshes per second.
)pbdoc");

    m.def(
        "AdafruitMatrixBonnetRGB565",
        [](py::buffer buffer, const piomatter::matrix_geometry &geometry) {
            return make_piomatter(Colorspace::RGB565,
                                  Pinout::AdafruitMatrixBonnet, buffer,
                                  geometry);
        },
        py::arg("buffer"), py::arg("geometry"),
        R"pbdoc(
Construct a PioMatter object to drive panels connected to an
Adafruit Matrix Bonnet using the RGB565 memory layout (2 bytes per
pixel)

This is deprecated shorthand for `PioMatter(Colorspace.RGB565, Pinout.AdafruitMatrixBonnet, ...)`.
)pbdoc");

    m.def(
        "AdafruitMatrixBonnetRGB888",
        [](py::buffer buffer, const piomatter::matrix_geometry &geometry) {
            return make_piomatter(Colorspace::RGB888,
                                  Pinout::AdafruitMatrixBonnet, buffer,
                                  geometry);
        },
        py::arg("framebuffer"), py::arg("geometry"),
        R"pbdoc(
Construct a PioMatter object to drive panels connected to an
Adafruit Matrix Bonnet using the RGB888 memory layout (4 bytes per
pixel)

This is deprecated shorthand for `PioMatter(Colorspace.RGB888, Pinout.AdafruitMatrixBonnet, ...)`.
)pbdoc")
        //.doc() =
        ;

    m.def(
        "AdafruitMatrixBonnetRGB888Packed",
        [](py::buffer buffer, const piomatter::matrix_geometry &geometry) {
            return make_piomatter(Colorspace::RGB888Packed,
                                  Pinout::AdafruitMatrixBonnet, buffer,
                                  geometry);
        },
        py::arg("framebuffer"), py::arg("geometry"),
        R"pbdoc(
Construct a PioMatter object to drive panels connected to an
Adafruit Matrix Bonnet using the RGB888 packed memory layout (3
bytes per pixel)

This is deprecated shorthand for `PioMatter(Colorspace.RGB888Packed, Pinout.AdafruitMatrixBonnet, ...)`.
)pbdoc");
}
