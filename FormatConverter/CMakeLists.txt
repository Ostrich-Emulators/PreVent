add_executable(formatconverter main.cpp Db.cpp)

execute_process (
    COMMAND wfdb-config "--libs"
    OUTPUT_VARIABLE WFDB_LIBRARIES
		OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process (
    COMMAND wfdb-config "--cflags"
    OUTPUT_VARIABLE WFDB_INCLUDES
		OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_link_libraries(formatconverter
	formats
	${SQLITE_LIBRARIES}
	${WFDB_LIBRARIES}
)

target_compile_options(formatconverter PRIVATE -Wall -O0)

target_include_directories(formatconverter
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}../Formats>
)
# need this because MATIO says it links to -lhdf5, but
# the HDF5 package uses a full path to find the library
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib/x86_64-linux-gnu/hdf5/serial/lib -L${TDMS_INSTALL_LOCATION}/lib")

install (PROGRAMS formatconverter DESTINATION bin)

