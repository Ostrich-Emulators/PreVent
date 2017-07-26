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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/DataRow.o \
	${OBJECTDIR}/Formats.o \
	${OBJECTDIR}/Hdf5Reader.o \
	${OBJECTDIR}/Hdf5Writer.o \
	${OBJECTDIR}/MatWriter.o \
	${OBJECTDIR}/ReadInfo.o \
	${OBJECTDIR}/Reader.o \
	${OBJECTDIR}/SignalData.o \
	${OBJECTDIR}/StpXmlReader.o \
	${OBJECTDIR}/StreamChunkReader.o \
	${OBJECTDIR}/WfdbReader.o \
	${OBJECTDIR}/WfdbWriter.o \
	${OBJECTDIR}/Writer.o \
	${OBJECTDIR}/ZlReader.o \
	${OBJECTDIR}/ZlWriter.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wno-deprecated -O2 `wfdb-config --cflags` 
CXXFLAGS=-Wno-deprecated -O2 `wfdb-config --cflags` 

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs zlib` /usr/lib/x86_64-linux-gnu/libsz.so /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_cpp.so `pkg-config --libs libxml-2.0`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter: /usr/lib/x86_64-linux-gnu/libsz.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_cpp.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter ${OBJECTFILES} ${LDLIBSOPTIONS} `wfdb-config --libs`

${OBJECTDIR}/DataRow.o: DataRow.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DataRow.o DataRow.cpp

${OBJECTDIR}/Formats.o: Formats.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Formats.o Formats.cpp

${OBJECTDIR}/Hdf5Reader.o: Hdf5Reader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Hdf5Reader.o Hdf5Reader.cpp

${OBJECTDIR}/Hdf5Writer.o: Hdf5Writer.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Hdf5Writer.o Hdf5Writer.cpp

${OBJECTDIR}/MatWriter.o: MatWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MatWriter.o MatWriter.cpp

${OBJECTDIR}/ReadInfo.o: ReadInfo.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ReadInfo.o ReadInfo.cpp

${OBJECTDIR}/Reader.o: Reader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Reader.o Reader.cpp

${OBJECTDIR}/SignalData.o: SignalData.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SignalData.o SignalData.cpp

${OBJECTDIR}/StpXmlReader.o: StpXmlReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/StpXmlReader.o StpXmlReader.cpp

${OBJECTDIR}/StreamChunkReader.o: StreamChunkReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/StreamChunkReader.o StreamChunkReader.cpp

${OBJECTDIR}/WfdbReader.o: WfdbReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WfdbReader.o WfdbReader.cpp

${OBJECTDIR}/WfdbWriter.o: WfdbWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WfdbWriter.o WfdbWriter.cpp

${OBJECTDIR}/Writer.o: Writer.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Writer.o Writer.cpp

${OBJECTDIR}/ZlReader.o: ZlReader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ZlReader.o ZlReader.cpp

${OBJECTDIR}/ZlWriter.o: ZlWriter.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ZlWriter.o ZlWriter.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 `pkg-config --cflags zlib` `pkg-config --cflags libxml-2.0` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsz.so ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhdf5.so ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhdf5_cpp.so
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
