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
CND_PLATFORM=Cygwin-Windows
CND_DLIB_EXT=dll
CND_CONF=Release_Cygwin
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Db.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wno-deprecated -O2
CXXFLAGS=-Wno-deprecated -O2

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../Formats/dist/Release_Cygwin/Cygwin-Windows -lFormats `pkg-config --libs sqlite3` -L../libtdms/dist/Release_cygwin/Cygwin-Windows -ltdms  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter.exe
	${CP} ../Formats/dist/Release_Cygwin/Cygwin-Windows/libFormats.dll ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${CP} ../libtdms/dist/Release_cygwin/Cygwin-Windows/libtdms.dll ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter.exe: ../Formats/dist/Release_Cygwin/Cygwin-Windows/libFormats.dll

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter.exe: ../libtdms/dist/Release_cygwin/Cygwin-Windows/libtdms.dll

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter.exe: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter ${OBJECTFILES} ${LDLIBSOPTIONS} -s

${OBJECTDIR}/Db.o: Db.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -s -I../Formats `pkg-config --cflags sqlite3` -std=c++14  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Db.o Db.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -s -I../Formats `pkg-config --cflags sqlite3` -std=c++14  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:
	cd ../Formats && ${MAKE}  -f Makefile CONF=Release_Cygwin
	cd ../libtdms && ${MAKE}  -f Makefile CONF=Release_cygwin

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libFormats.dll ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libtdms.dll
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/formatconverter.exe

# Subprojects
.clean-subprojects:
	cd ../Formats && ${MAKE}  -f Makefile CONF=Release_Cygwin clean
	cd ../libtdms && ${MAKE}  -f Makefile CONF=Release_cygwin clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
