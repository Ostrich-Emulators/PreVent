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

include_directories(${SQLITE_INCLUDE_DIRS})

# need this because MATIO says it links to -lhdf5, but
# the HDF5 package uses a full path to find the library
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib/x86_64-linux-gnu/hdf5/serial/lib -L${TDMS_INSTALL_LOCATION}/lib")

install (PROGRAMS formatconverter DESTINATION bin)

