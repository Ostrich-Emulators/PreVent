set(CMAKE_CXX_COMPILER /usr/bin/g++)
cmake_minimum_required(VERSION 3.10)

project(formatconverter)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib:$ORIGIN/")

set(HDF5_USE_STATIC_LIBRARIES false)
find_package(PkgConfig)
find_package(HDF5 REQUIRED COMPONENTS C CXX)
find_package(Git REQUIRED)
find_package(tdmspp-osem REQUIRED)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

pkg_check_modules (EXPAT REQUIRED expat>=2)
pkg_check_modules (SQLITE REQUIRED sqlite3)
pkg_check_modules (MATIO REQUIRED matio)

add_subdirectory(Formats)
add_subdirectory(PreVentTools)
add_subdirectory(FormatConverter)