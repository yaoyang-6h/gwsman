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
CC=mips-openwrt-linux-gcc
CCC=mips-openwrt-linux-g++
CXX=mips-openwrt-linux-g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU_MIPS32-Linux-x86
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
	${OBJECTDIR}/src/GuiMain.o \
	${OBJECTDIR}/src/PipeShell.o \
	${OBJECTDIR}/src/SvrMain.o \
	${OBJECTDIR}/src/SvrMon.o \
	${OBJECTDIR}/src/gwsbuffer.o \
	${OBJECTDIR}/src/gwsman.o \
	${OBJECTDIR}/src/gwsmanlib.o \
	${OBJECTDIR}/src/iw/bitrate.o \
	${OBJECTDIR}/src/iw/connect.o \
	${OBJECTDIR}/src/iw/cqm.o \
	${OBJECTDIR}/src/iw/event.o \
	${OBJECTDIR}/src/iw/genl.o \
	${OBJECTDIR}/src/iw/hwsim.o \
	${OBJECTDIR}/src/iw/ibss.o \
	${OBJECTDIR}/src/iw/info.o \
	${OBJECTDIR}/src/iw/interface.o \
	${OBJECTDIR}/src/iw/iw.o \
	${OBJECTDIR}/src/iw/link.o \
	${OBJECTDIR}/src/iw/mesh.o \
	${OBJECTDIR}/src/iw/mpath.o \
	${OBJECTDIR}/src/iw/offch.o \
	${OBJECTDIR}/src/iw/phy.o \
	${OBJECTDIR}/src/iw/ps.o \
	${OBJECTDIR}/src/iw/reason.o \
	${OBJECTDIR}/src/iw/reg.o \
	${OBJECTDIR}/src/iw/roc.o \
	${OBJECTDIR}/src/iw/scan.o \
	${OBJECTDIR}/src/iw/sections.o \
	${OBJECTDIR}/src/iw/station.o \
	${OBJECTDIR}/src/iw/status.o \
	${OBJECTDIR}/src/iw/survey.o \
	${OBJECTDIR}/src/iw/util.o \
	${OBJECTDIR}/src/iw/version.o \
	${OBJECTDIR}/src/iw/wowlan.o \
	${OBJECTDIR}/src/lcd_comm.o \
	${OBJECTDIR}/src/lcd_dev.o \
	${OBJECTDIR}/src/lcd_sp.o \
	${OBJECTDIR}/src/lcd_sp4lcd.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_lib.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_lua.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_madwifi.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_nl80211.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_utils.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_wext.o \
	${OBJECTDIR}/src/libiwinfo/iwinfo_wext_scan.o \
	${OBJECTDIR}/src/routeNeg.o \
	${OBJECTDIR}/src/shmem.o \
	${OBJECTDIR}/src/socketbase.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-Lsrc -lm libs/libreadconf.a libs/libserialcom.a libs/liblua.a libs/libnl.a libs/libnl-genl.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman: libs/libreadconf.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman: libs/libserialcom.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman: libs/liblua.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman: libs/libnl.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman: libs/libnl-genl.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman ${OBJECTFILES} ${LDLIBSOPTIONS} -lpthread

${OBJECTDIR}/src/GuiMain.o: src/GuiMain.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/GuiMain.o src/GuiMain.c

${OBJECTDIR}/src/PipeShell.o: src/PipeShell.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/PipeShell.o src/PipeShell.c

${OBJECTDIR}/src/SvrMain.o: src/SvrMain.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/SvrMain.o src/SvrMain.c

${OBJECTDIR}/src/SvrMon.o: src/SvrMon.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/SvrMon.o src/SvrMon.c

${OBJECTDIR}/src/gwsbuffer.o: src/gwsbuffer.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gwsbuffer.o src/gwsbuffer.c

${OBJECTDIR}/src/gwsman.o: src/gwsman.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gwsman.o src/gwsman.c

${OBJECTDIR}/src/gwsmanlib.o: src/gwsmanlib.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/gwsmanlib.o src/gwsmanlib.c

${OBJECTDIR}/src/iw/bitrate.o: src/iw/bitrate.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/bitrate.o src/iw/bitrate.c

${OBJECTDIR}/src/iw/connect.o: src/iw/connect.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/connect.o src/iw/connect.c

${OBJECTDIR}/src/iw/cqm.o: src/iw/cqm.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/cqm.o src/iw/cqm.c

${OBJECTDIR}/src/iw/event.o: src/iw/event.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/event.o src/iw/event.c

${OBJECTDIR}/src/iw/genl.o: src/iw/genl.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/genl.o src/iw/genl.c

${OBJECTDIR}/src/iw/hwsim.o: src/iw/hwsim.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/hwsim.o src/iw/hwsim.c

${OBJECTDIR}/src/iw/ibss.o: src/iw/ibss.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/ibss.o src/iw/ibss.c

${OBJECTDIR}/src/iw/info.o: src/iw/info.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/info.o src/iw/info.c

${OBJECTDIR}/src/iw/interface.o: src/iw/interface.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/interface.o src/iw/interface.c

${OBJECTDIR}/src/iw/iw.o: src/iw/iw.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/iw.o src/iw/iw.c

${OBJECTDIR}/src/iw/link.o: src/iw/link.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/link.o src/iw/link.c

${OBJECTDIR}/src/iw/mesh.o: src/iw/mesh.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/mesh.o src/iw/mesh.c

${OBJECTDIR}/src/iw/mpath.o: src/iw/mpath.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/mpath.o src/iw/mpath.c

${OBJECTDIR}/src/iw/offch.o: src/iw/offch.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/offch.o src/iw/offch.c

${OBJECTDIR}/src/iw/phy.o: src/iw/phy.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/phy.o src/iw/phy.c

${OBJECTDIR}/src/iw/ps.o: src/iw/ps.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/ps.o src/iw/ps.c

${OBJECTDIR}/src/iw/reason.o: src/iw/reason.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/reason.o src/iw/reason.c

${OBJECTDIR}/src/iw/reg.o: src/iw/reg.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/reg.o src/iw/reg.c

${OBJECTDIR}/src/iw/roc.o: src/iw/roc.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/roc.o src/iw/roc.c

${OBJECTDIR}/src/iw/scan.o: src/iw/scan.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/scan.o src/iw/scan.c

${OBJECTDIR}/src/iw/sections.o: src/iw/sections.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/sections.o src/iw/sections.c

${OBJECTDIR}/src/iw/station.o: src/iw/station.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/station.o src/iw/station.c

${OBJECTDIR}/src/iw/status.o: src/iw/status.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/status.o src/iw/status.c

${OBJECTDIR}/src/iw/survey.o: src/iw/survey.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/survey.o src/iw/survey.c

${OBJECTDIR}/src/iw/util.o: src/iw/util.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/util.o src/iw/util.c

${OBJECTDIR}/src/iw/version.o: src/iw/version.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/version.o src/iw/version.c

${OBJECTDIR}/src/iw/wowlan.o: src/iw/wowlan.c 
	${MKDIR} -p ${OBJECTDIR}/src/iw
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/iw/wowlan.o src/iw/wowlan.c

${OBJECTDIR}/src/lcd_comm.o: src/lcd_comm.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/lcd_comm.o src/lcd_comm.c

${OBJECTDIR}/src/lcd_dev.o: src/lcd_dev.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/lcd_dev.o src/lcd_dev.c

${OBJECTDIR}/src/lcd_sp.o: src/lcd_sp.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/lcd_sp.o src/lcd_sp.c

${OBJECTDIR}/src/lcd_sp4lcd.o: src/lcd_sp4lcd.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/lcd_sp4lcd.o src/lcd_sp4lcd.c

${OBJECTDIR}/src/libiwinfo/iwinfo_lib.o: src/libiwinfo/iwinfo_lib.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_lib.o src/libiwinfo/iwinfo_lib.c

${OBJECTDIR}/src/libiwinfo/iwinfo_lua.o: src/libiwinfo/iwinfo_lua.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_lua.o src/libiwinfo/iwinfo_lua.c

${OBJECTDIR}/src/libiwinfo/iwinfo_madwifi.o: src/libiwinfo/iwinfo_madwifi.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_madwifi.o src/libiwinfo/iwinfo_madwifi.c

${OBJECTDIR}/src/libiwinfo/iwinfo_nl80211.o: src/libiwinfo/iwinfo_nl80211.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_nl80211.o src/libiwinfo/iwinfo_nl80211.c

${OBJECTDIR}/src/libiwinfo/iwinfo_utils.o: src/libiwinfo/iwinfo_utils.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_utils.o src/libiwinfo/iwinfo_utils.c

${OBJECTDIR}/src/libiwinfo/iwinfo_wext.o: src/libiwinfo/iwinfo_wext.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_wext.o src/libiwinfo/iwinfo_wext.c

${OBJECTDIR}/src/libiwinfo/iwinfo_wext_scan.o: src/libiwinfo/iwinfo_wext_scan.c 
	${MKDIR} -p ${OBJECTDIR}/src/libiwinfo
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/libiwinfo/iwinfo_wext_scan.o src/libiwinfo/iwinfo_wext_scan.c

${OBJECTDIR}/src/routeNeg.o: src/routeNeg.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/routeNeg.o src/routeNeg.c

${OBJECTDIR}/src/shmem.o: src/shmem.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/shmem.o src/shmem.c

${OBJECTDIR}/src/socketbase.o: src/socketbase.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -DCONFIG_LIBNL20 -Iinclude -Isrc/libiwinfo/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/libnl-2.0/include -I../../build_dir/target-mips_r2_uClibc-0.9.33.2/lua-5.1.4/ipkg-install/usr/include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/socketbase.o src/socketbase.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/gwsman

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
