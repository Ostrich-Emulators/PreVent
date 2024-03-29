# this doesn't seem to work like it should...
# the hdf5c++ wrapper has some incompatibility with cmake, I think
#set(CMAKE_CXX_COMPILER ${HDF5_CXX_COMPILER_EXECUTABLE})

set(LIBVERSION 4.3.12)
project(formatconverter VERSION ${LIBVERSION} LANGUAGES CXX)

pkg_check_modules( ZIP REQUIRED libzip )

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
#
#
#MESSAGE ( ${ZIP_VERSION} )
#MESSAGE ( ${ZIP_INCLUDEDIR} )
#MESSAGE ( ${ZIP_LIBDIR} )
#
#MESSAGE ( ${ZLIB_LIBRARIES} )
#MESSAGE ( ${ZLIB_LIBRARY_DIRS} )
#MESSAGE ( ${ZLIB_LDFLAGS} )
#MESSAGE ( ${ZLIB_INCLUDE_DIRS} )
#MESSAGE ( ${ZLIB_CFLAGS} )
#
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib/formatconverter-${LIBVERSION}:$ORIGIN/")

# add the command to generate the source code
execute_process (
	OUTPUT_VARIABLE GIT_HASH
	OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND git describe --abbrev=8 --dirty --always --match goobeligook
)
MESSAGE( STATUS "the git hash: ${GIT_HASH}" )

if( MSVC)
  
else()

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
endif()

configure_file (
  "${CMAKE_SOURCE_DIR}/Formats/config.h.in"
  "${CMAKE_SOURCE_DIR}/Formats/generated/config.h"
)

add_custom_command(
  OUTPUT generated/releases.h
  COMMAND xxd -i "releases.h.in" generated/releases.h
  DEPENDS "releases.h.in"
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
TimeModifier.cpp
TimeParser.cpp
StpReaderBase.cpp
StpGeReader.cpp
StpGeSegment.cpp
StpPhilipsReader.cpp
DwcReader.cpp
NullWriter.cpp
NullSignalData.cpp
Log.cpp
AutonWriter.cpp
CsvReader.cpp
Csv2Reader.cpp
SplitLogic.cpp
DWCxReader.cpp
generated/config.h
generated/releases.h
)

set(include_dest include/formatconverter-${LIBVERSION})
set(lib_dest lib/formatconverter-${LIBVERSION})

target_compile_options(formats PRIVATE -Wall)

target_include_directories(formats
    PUBLIC
        $<INSTALL_INTERFACE:${include_dest}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        $<INSTALL_INTERFACE:${include_dest}>
        $<BUILD_INTERFACE:generated ${HDF5_INCLUDE_DIRS} ${MATIO_INCLUDES} ${WFDB_INCLUDES} ${EXPAT_INCLUDE_DIR} ${ZIP_INCLUDEDIR}>
)

target_link_libraries(formats 
	tdmspp-osem
	stdc++fs
	${EXPAT_LIBRARIES}
	${MATIO_LIBRARIES}
	${HDF5_LIBRARIES}
	${WFDB_LIBRARIES}
	${ZIP_LINK_LIBRARIES}
  ${ZLIB_LIBRARIES}
	# need this because MATIO says it links to -lhdf5, but
	# the HDF5 package uses a full path to find the library
	"-L/usr/lib/x86_64-linux-gnu/hdf5/serial"
)

install (TARGETS formats DESTINATION ${lib_dest})
install (FILES AnonymizingSignalSet.h
exports.h
Base64.h
BasicSignalData.h
BasicSignalSet.h
generated/config.h
ConversionListener.h
CpcXmlReader.h
CsvWriter.h
DataRow.h
dr_time.h
FileCachingVector.h
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
SignalDataWrapper.h
SignalSet.h
SignalSetWrapper.h
SignalUtils.h
StpJsonReader.h
StpXmlReader.h
StreamChunkReader.h
TdmsReader.h
TimeRange.h
TimezoneOffsetTimeSignalSet.h
WfdbReader.h
WfdbWriter.h
Writer.h
XmlReaderBase.h
ZlReader2.h
ZlReader.h
StpReaderBase.h
StpGeReader.h
StpGeSegment.h
StpPhilipsReader.h
DwcReader.h
zstr.hpp
strict_fstream.hpp
CircularBuffer.h
NullWriter.h
NullSignalData.h
Log.h
AutonWriter.h
CsvReader.h
Csv2Reader.h
SplitLogic.h
DWCxReader.h
generated/releases.h
DESTINATION ${include_dest} )
