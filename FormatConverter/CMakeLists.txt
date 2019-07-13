#set(CMAKE_CXX_COMPILER ${HDF5_CXX_COMPILER_EXECUTABLE})

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

target_link_libraries(formatconverter tdmspp-osem)
target_link_libraries(formatconverter formats)
target_link_libraries(formatconverter stdc++fs)
target_link_libraries(formatconverter ${SQLITE_LIBRARIES})
target_link_libraries(formatconverter ${EXPAT_LIBRARIES})
target_link_libraries(formatconverter ${MATIO_LIBRARIES})
target_link_libraries(formatconverter ${HDF5_LIBRARIES})
target_link_libraries(formatconverter ${WFDB_LIBRARIES})

include_directories(${SQLITE_INCLUDE_DIRS})

# need this because MATIO says it links to -lhdf5, but
# the HDF5 package uses a full path to find the library
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib/x86_64-linux-gnu/hdf5/serial/lib")
