cmake_minimum_required(VERSION 3.4...3.18)
project("_piomatter")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)

find_package(pybind11 CONFIG)
pybind11_add_module("_piomatter"
    src/pymain.cpp src/piolib/pio_rp1.c src/piolib/piolib.c)
target_include_directories("_piomatter" PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
    ${CMAKE_CURRENT_LIST_DIR}/src/piolib/include)
set_property(TARGET "_piomatter" PROPERTY
    CXX_STANDARD 20)
