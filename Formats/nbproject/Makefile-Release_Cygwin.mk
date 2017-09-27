#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=h5c++
CXX=h5c++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release_Cygwin
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Base64.o \
	${OBJECTDIR}/CpcXmlReader.o \
	${OBJECTDIR}/CsvWriter.o \
	${OBJECTDIR}/DataRow.o \
	${OBJECTDIR}/Formats.o \
	${OBJECTDIR}/Hdf5Reader.o \
	${OBJECTDIR}/Hdf5Writer.o \
	${OBJECTDIR}/MatWriter.o \
	${OBJECTDIR}/Reader.o \
	${OBJECTDIR}/SignalData.o \
	${OBJECTDIR}/SignalSet.o \
	${OBJECTDIR}/SignalUtils.o \
	${OBJECTDIR}/StpXmlReader.o \
	${OBJECTDIR}/StreamChunkReader.o \
	${OBJECTDIR}/WfdbReader.o \
	${OBJECTDIR}/WfdbWriter.o \
	${OBJECTDIR}/Writer.o \
	${OBJECTDIR}/XmlReaderBase.o \
	${OBJECTDIR}/ZlReader.o \
	${OBJECTDIR}/ZlWriter.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wno-deprecated `wfdb-config --cflags` -D_XOPEN_SOURCE=700 
CXXFLAGS=-Wno-deprecated `wfdb-config --cflags` -D_XOPEN_SOURCE=700 

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs zlib` -lhdf5.dll -lhdf5_cpp.dll `pkg-config --libs libxml-2.0` `pkg-config --libs matio`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFormats.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFormats.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	g++ -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFormats.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} `wfdb-config --libs` -shared -fPIC

${OBJECTDIR}/Base64.o: Base64.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Base64.o Base64.cpp

${OBJECTDIR}/CpcXmlReader.o: CpcXmlReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CpcXmlReader.o CpcXmlReader.cpp

${OBJECTDIR}/CsvWriter.o: CsvWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CsvWriter.o CsvWriter.cpp

${OBJECTDIR}/DataRow.o: DataRow.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DataRow.o DataRow.cpp

${OBJECTDIR}/Formats.o: Formats.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Formats.o Formats.cpp

${OBJECTDIR}/Hdf5Reader.o: Hdf5Reader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Hdf5Reader.o Hdf5Reader.cpp

${OBJECTDIR}/Hdf5Writer.o: Hdf5Writer.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Hdf5Writer.o Hdf5Writer.cpp

${OBJECTDIR}/MatWriter.o: MatWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MatWriter.o MatWriter.cpp

${OBJECTDIR}/Reader.o: Reader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Reader.o Reader.cpp

${OBJECTDIR}/SignalData.o: SignalData.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SignalData.o SignalData.cpp

${OBJECTDIR}/SignalSet.o: SignalSet.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SignalSet.o SignalSet.cpp

${OBJECTDIR}/SignalUtils.o: SignalUtils.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SignalUtils.o SignalUtils.cpp

${OBJECTDIR}/StpXmlReader.o: StpXmlReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/StpXmlReader.o StpXmlReader.cpp

${OBJECTDIR}/StreamChunkReader.o: StreamChunkReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/StreamChunkReader.o StreamChunkReader.cpp

${OBJECTDIR}/WfdbReader.o: WfdbReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WfdbReader.o WfdbReader.cpp

${OBJECTDIR}/WfdbWriter.o: WfdbWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WfdbWriter.o WfdbWriter.cpp

${OBJECTDIR}/Writer.o: Writer.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Writer.o Writer.cpp

${OBJECTDIR}/XmlReaderBase.o: XmlReaderBase.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/XmlReaderBase.o XmlReaderBase.cpp

${OBJECTDIR}/ZlReader.o: ZlReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ZlReader.o ZlReader.cpp

${OBJECTDIR}/ZlWriter.o: ZlWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` `pkg-config --cflags matio` -std=c++14  -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ZlWriter.o ZlWriter.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
