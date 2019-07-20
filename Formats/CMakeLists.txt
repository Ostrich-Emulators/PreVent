# this doesn't seem to work like it should...
# the hdf5c++ wrapper has some incompatibility with cmake, I think
#set(CMAKE_CXX_COMPILER ${HDF5_CXX_COMPILER_EXECUTABLE})

# add the command to generate the source code
execute_process (
	OUTPUT_VARIABLE GIT_HASH
	OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND git describe --abbrev=8 --dirty --always --tags
)

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

MESSAGE( STATUS "the git hash: ${GIT_HASH}" )

configure_file (
  "build.h.in"
  "generated/build.h"
)

add_library(formats SHARED AnonymizingSignalSet.cpp
Base64.cpp
BasicSignalData.cpp
BasicSignalSet.cpp
CpcXmlReader.cpp
CsvWriter.cpp
DataRow.cpp
FileNamer.cpp
Formats.cpp
Hdf5Reader.cpp
Hdf5Writer.cpp
MatWriter.cpp
NullReader.cpp
OffsetTimeSignalSet.cpp
Options.cpp
Reader.cpp
SignalData.cpp
SignalDataWrapper.cpp
SignalSet.cpp
SignalSetWrapper.cpp
SignalUtils.cpp
StpJsonReader.cpp
StpXmlReader.cpp
StreamChunkReader.cpp
TdmsReader.cpp
TimezoneOffsetTimeSignalSet.cpp
WfdbReader.cpp
WfdbWriter.cpp
Writer.cpp
XmlReaderBase.cpp
ZlReader2.cpp
ZlReader.cpp
generated/build.h
)

target_compile_options(formats PRIVATE -Wall)

target_include_directories(formats
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:generated ${HDF5_INCLUDE_DIRS} ${MATIO_INCLUDES} ${WFDB_INCLUDES}>
)

target_link_libraries(formats 
	tdmspp-osem
	stdc++fs
	${EXPAT_LIBRARIES}
	${MATIO_LIBRARIES}
	${HDF5_LIBRARIES}
	${WFDB_LIBRARIES}
	# need this because MATIO says it links to -lhdf5, but
	# the HDF5 package uses a full path to find the library
	"-L/usr/lib/x86_64-linux-gnu/hdf5/serial/lib"
)

install (TARGETS formats DESTINATION lib)
install (FILES AnonymizingSignalSet.h
Base64.h
BasicSignalData.h
BasicSignalSet.h
generated/build.h
config.h
ConversionListener.h
CpcXmlReader.h
CsvWriter.h
DataRow.h
dr_time.h
FileNamer.h
Formats.h
Hdf5Reader.h
Hdf5Writer.h
MatWriter.h
NullReader.h
OffsetTimeSignalSet.h
Options.h
Reader.h
SignalData.h
SignalDataIterator.h
SignalDataWrapper.h
SignalSet.h
SignalSetWrapper.h
SignalUtils.h
StpJsonReader.h
StpXmlReader.h
StreamChunkReader.h
TdmsReader.h
TimezoneOffsetTimeSignalSet.h
WfdbReader.h
WfdbWriter.h
Writer.h
XmlReaderBase.h
ZlReader2.h
ZlReader.h
DESTINATION include )
