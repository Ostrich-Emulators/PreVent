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
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/ChannelOffsetInfo.o \
	${OBJECTDIR}/TdmsChannel.o \
	${OBJECTDIR}/TdmsGroup.o \
	${OBJECTDIR}/TdmsLeadIn.o \
	${OBJECTDIR}/TdmsMetaData.o \
	${OBJECTDIR}/TdmsObject.o \
	${OBJECTDIR}/TdmsParser.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-g -O0 -D_XOPEN_SOURCE=700
CXXFLAGS=-g -O0 -D_XOPEN_SOURCE=700

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libtdms.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libtdms.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libtdms.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/ChannelOffsetInfo.o: ChannelOffsetInfo.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ChannelOffsetInfo.o ChannelOffsetInfo.cpp

${OBJECTDIR}/TdmsChannel.o: TdmsChannel.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TdmsChannel.o TdmsChannel.cpp

${OBJECTDIR}/TdmsGroup.o: TdmsGroup.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TdmsGroup.o TdmsGroup.cpp

${OBJECTDIR}/TdmsLeadIn.o: TdmsLeadIn.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TdmsLeadIn.o TdmsLeadIn.cpp

${OBJECTDIR}/TdmsMetaData.o: TdmsMetaData.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TdmsMetaData.o TdmsMetaData.cpp

${OBJECTDIR}/TdmsObject.o: TdmsObject.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TdmsObject.o TdmsObject.cpp

${OBJECTDIR}/TdmsParser.o: TdmsParser.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iinclude -std=c++14 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TdmsParser.o TdmsParser.cpp

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
