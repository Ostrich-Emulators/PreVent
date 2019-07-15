project(formatconverter)

##### RPATH stuff #####

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)


# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")


cmake_minimum_required(VERSION 3.10)
set(HDF5_USE_STATIC_LIBRARIES false)
find_package(PkgConfig)
find_package(HDF5 REQUIRED COMPONENTS C CXX)
find_package(Git REQUIRED)
#find_package(ExternalProject)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

pkg_check_modules (EXPAT REQUIRED expat>=2)
pkg_check_modules (SQLITE REQUIRED sqlite3)
pkg_check_modules (MATIO REQUIRED matio)

# get the TDMSpp-osem project from git
set(TDMS_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/generated/external)
include(ExternalProject)
ExternalProject_Add(TDMSpp-osem
    GIT_REPOSITORY https://github.com/Ostrich-Emulators/TDMSpp
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)


include_directories(${TDMS_INSTALL_LOCATION}/include)
include_directories(Formats)
include_directories(Formats/generated)

add_subdirectory(Formats)
add_subdirectory(FormatConverter)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O0")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
