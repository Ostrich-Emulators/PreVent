
cmake_minimum_required(VERSION 3.10)

project(formatconverter)

if(CYGWIN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_XOPEN_SOURCE=700")
endif()

if( MSVC )
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3")
else()
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -O0")
  set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -O0")

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -pg -g -O3")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} -pg -g -O3")
  set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_RELWITHDEBINFO} -pg -g -O3")
  
  target_compile_options(formats PRIVATE -Wall)
endif()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH true)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib:$ORIGIN/")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(HDF5 REQUIRED COMPONENTS C CXX)
find_package(Git REQUIRED)

if(MSVC)
  # find dlls somehow
  set(tdmspp-osem_DIR "../Tdmspp/out/install/x64-Debug/lib/tdmspp-osem-2.0.0")
  set(expat_DIR "C:/Program Files (x86)/Expat 2.2.0")
  set(SQLITE_LIBRARIES "C:/Program Files (x86)/sqlite3/sqlite3.dll")
  find_package(EXPAT REQUIRED expat2>=2)
else()
set(HDF5_USE_STATIC_LIBRARIES false)
  find_package(PkgConfig)
  pkg_check_modules (EXPAT REQUIRED expat>=2)
  pkg_check_modules (SQLITE REQUIRED sqlite3)
  pkg_check_modules (MATIO REQUIRED matio)
endif()

find_package(tdmspp-osem 2 REQUIRED)

add_subdirectory(Formats)
add_subdirectory(PreVentTools)
add_subdirectory(FormatConverter)
