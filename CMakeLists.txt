cmake_minimum_required(VERSION 2.2)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0002 OLD)
endif(COMMAND cmake_policy)


INCLUDE(cmake/initng_module.cmake)

PROJECT(initng C)

SET(VERSION "0.7.0-svn" CACHE STRING "Version number of the project")
SET(VERSION_NAME "Bleeding Edge" CACHE STRING "Version string")

SET(API_VERSION 0.7)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(FindPkgConfig)
INCLUDE(CheckSymbolExists)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
# Turn on warnings
SET(CMAKE_C_FLAGS "-std=c99 -Wall -O2 -D_XOPEN_SOURCE=600 ${CMAKE_C_FLAGS}")
SET(MOD_CFLAGS "${CMAKE_C_FLAGS}")
SET(MOD_LDFLAGS "${CMAKE_LINK_FLAGS}")

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}
)

# Colorize output, needs CMake > 2.3.4
OPTION(CMAKE_COLOR_MAKEFILE "Generate Makefiles with colored output" ON)

SET(CMAKE_INSTALL_PREFIX "" CACHE INTERNAL "")
SET(INCLUDE_INSTALL_DIR /usr/include/libinitng-${API_VERSION} CACHE STRING
	"Install location for header files.")
SET(SBIN_INSTALL_DIR /sbin CACHE STRING "Install location for system binaries.")
SET(BIN_INSTALL_DIR /bin CACHE STRING "Install location for binaries.")
SET(LIB_INSTALL_DIR /lib CACHE STRING "Install location for libraries.")
SET(SYSCONF_INSTALL_DIR /etc CACHE STRING "Install location for configuration files.")
SET(DATA_INSTALL_DIR /usr/share CACHE STRING "Install location for data files.")
SET(MAN_INSTALL_DIR ${DATA_INSTALL_DIR}/man CACHE STRING "Install location for manpages.")
SET(DOC_INSTALL_DIR ${DATA_INSTALL_DIR}/doc CACHE STRING "Install location for documentation.")
SET(PKGCONFIG_INSTALL_DIR /usr/lib/pkgconfig CACHE STRING "Install location for pkg-config files.")
MARK_AS_ADVANCED(INCLUDE_INSTALL_DIR)
MARK_AS_ADVANCED(SBIN_INSTALL_DIR)
MARK_AS_ADVANCED(BIN_INSTALL_DIR)
MARK_AS_ADVANCED(LIB_INSTALL_DIR)
MARK_AS_ADVANCED(SYSCONF_INSTALL_DIR)
MARK_AS_ADVANCED(DATA_INSTALL_DIR)
MARK_AS_ADVANCED(MAN_INSTALL_DIR)
MARK_AS_ADVANCED(DOC_INSTALL_DIR)
MARK_AS_ADVANCED(PKGCONFIG_INSTALL_DIR)

SET(RUNLEVEL_DEFAULT runlevel/default CACHE STRING "Default runlevel.")
MARK_AS_ADVANCED(RUNLEVEL_DEFAULT)

pkg_search_module(DBUS dbus-1)
SET(DBUS_DEFINITIONS ${DBUS_CFLAGS})
FIND_LIBRARY(DBUS_LIBS NAMES dbus-1 PATHS ${DBUS_LINK_DIR} /usr/lib /usr/local/lib)

SUBDIRS(
	doc
	extras
	core
	modules
	tools
	preinit
	include
)

OPTION(BUILD_ALSO "Build also module" ON)
OPTION(BUILD_SERVICE_FILE "Build service_file module" ON )
OPTION(BUILD_CHDIR "Build chdir module" ON)
OPTION(BUILD_CHROOT "Build chroot module" ON)
OPTION(BUILD_CONFLICT "Build conflict module" ON)
OPTION(BUILD_CPOUT "Build cpout module" ON)
OPTION(BUILD_CRITICAL "Build critical module" ON)
OPTION(BUILD_CTRLALTDEL "Build ctrlaltdel module" ON)
OPTION(BUILD_DAEMON_CLEAN "Build daemon_clean module" OFF)
OPTION(BUILD_DBUS_EVENT "Build dbus_event module" OFF)
OPTION(BUILD_DEBUG_COMMANDS "Build debug_commands module" ON)
OPTION(BUILD_TASK "Build task service type module" ON)
OPTION(BUILD_FMON "Build fmon module" ON)
OPTION(BUILD_FSTAT "Build fstat module" ON)
OPTION(BUILD_HISTORY "Build history module" OFF)
OPTION(BUILD_INITCTL "Build initctl module" ON)
OPTION(BUILD_INTERACTIVE "Build interactive module" ON)
OPTION(BUILD_LAST "Build last module" ON)
OPTION(BUILD_LIMIT "Build limit module" ON)
OPTION(BUILD_LOGFILE "Build logfile module" ON)
OPTION(BUILD_LOCKFILE "Build lockfile module" ON)
OPTION(BUILD_NETDEV "Build netdev module" ON)
OPTION(BUILD_IDLEPROBE "Build idleprobe module" ON)
OPTION(BUILD_NGC4 "Build ngc4 module" ON)
OPTION(BUILD_NGCS "Build ngcs module" OFF)
OPTION(BUILD_NGE "Build nge module" ON)
OPTION(BUILD_PAUSE "Build pause module" ON)
OPTION(BUILD_PROVIDE "Build provide module" ON)
OPTION(BUILD_RELOAD "Build reload module" ON)
OPTION(BUILD_RENICE "Build renice module" ON)
OPTION(BUILD_SIMPLE_LAUNCHER "Build simple_launcher module" ON)
OPTION(BUILD_STCMD "Build stcmd module" ON)
OPTION(BUILD_STDOUT "Build stdout module" ON)
OPTION(BUILD_SUID "Build suid module" ON)
OPTION(BUILD_SYNCRON "Build syncron module" ON)
OPTION(BUILD_SYSLOG "Build syslog module" ON)
OPTION(BUILD_SYSREQ "Build sysreq module" ON)
OPTION(BUILD_UNNEEDED "Build unneeded module" ON)
OPTION(BUILD_USPLASH "Build initng with usplash support" OFF)

OPTION(SELINUX_SUPPORT "SELinux support" OFF)
OPTION(INSTALL_AS_INIT "Install initng as complete replacement for SysVInit" OFF)
OPTION(DEBUG "Build with developer tools, turns on additional warnings and -Werror" OFF)
OPTION(CHECK_RO "Test if the / is mounted read-only before reboot/halt" ON)
OPTION(FORCE_NOCOLOR "Prevent cpout module from using colors" OFF)
OPTION(FORCE_POSIX_IFILES "Force POSIX compliance on ifiles" OFF)

IF(SELINUX_SUPPORT)
	CHECK_INCLUDE_FILES(selinux/selinux.h HAVE_SELINUX_H)
	CHECK_LIBRARY_EXISTS(selinux selinux_init_load_policy "" HAVE_SELINUX)
	IF(HAVE_SELINUX_H AND HAVE_SELINUX)
		ADD_DEFINITIONS(-DSELINUX)
		SET(SELINUX_LIBS selinux)
	ELSE(HAVE_SELINUX_H AND HAVE_SELINUX)
		MESSAGE(SEND_ERROR "Could not find selinux development files. Install the selinux devel package.")
	ENDIF(HAVE_SELINUX_H AND HAVE_SELINUX)
ENDIF(SELINUX_SUPPORT)

IF(DEBUG)
	CHECK_INCLUDE_FILES(google/coredumper.h HAVE_COREDUMPER_H)
	CHECK_LIBRARY_EXISTS(coredumper WriteCoreDump "" HAVE_COREDUMPER)

	IF(HAVE_COREDUMPER AND HAVE_COREDUMPER_H)
        	ADD_DEFINITIONS(-DHAVE_COREDUMPER)
		SET(COREDUMPER_LIBS coredumper)
	ENDIF(HAVE_COREDUMPER AND HAVE_COREDUMPER_H)

	SET(CMAKE_C_FLAGS "-DDEBUG -g -Werror -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wimplicit -Wredundant-decls -Wnested-externs -Wwrite-strings -Wsign-compare -Winline -Wswitch -Wreturn-type -Wparentheses -Wmissing-braces -Wformat -Wformat-nonliteral -Wformat-security -Wsequence-point -Wundef -Wunused -Wcomment ${CMAKE_C_FLAGS}")
ENDIF(DEBUG)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/initng.pc.in
	       ${CMAKE_CURRENT_BINARY_DIR}/initng-${API_VERSION}.pc
	       @ONLY)
INSTALL_FILES(${PKGCONFIG_INSTALL_DIR} FILES initng-${API_VERSION}.pc)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/initng-paths.h.in
	       ${CMAKE_CURRENT_BINARY_DIR}/initng-paths.h @ONLY)
INSTALL_FILES(${INCLUDE_INSTALL_DIR} FILES initng-paths.h)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
