# this doesn't seem to work like it should...
# the hdf5c++ wrapper has some incompatibility with cmake, I think
#set(CMAKE_CXX_COMPILER ${HDF5_CXX_COMPILER_EXECUTABLE})

# add the command to generate the source code
execute_process (
	OUTPUT_VARIABLE GIT_HASH
	OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND git describe --abbrev=8 --dirty --always --tags

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
build.h
)

include_directories(${HDF5_INCLUDE_DIRS})
include_directories(${WFDB_INCLUDES})
include_directories(generated)

target_link_libraries(formats 
	tdmspp-osem
	stdc++fs
	${EXPAT_LIBRARIES}
	${MATIO_LIBRARIES}
	${HDF5_LIBRARIES}
	${WFDB_LIBRARIES}
	# need these last two to find the libraries
	"-L${TDMS_INSTALL_LOCATION}/lib"
	# need this because MATIO says it links to -lhdf5, but
	# the HDF5 package uses a full path to find the library
	"-L/usr/lib/x86_64-linux-gnu/hdf5/serial/lib"
)

install (TARGETS formats DESTINATION lib)
