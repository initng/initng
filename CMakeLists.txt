PROJECT(initng C)

SET(VERSION "0.7.0-svn" CACHE STRING "Version number of the project")
SET(VERSION_NAME "Bleeding Edge" CACHE STRING "Version string")

IF(CMAKE_BACKWARDS_COMPATIBILITY LESS 2.2)
	MESSAGE(FATAL_ERROR "Error: Your CMake version is too old. This project requires version 2.2 or newer.")
ENDIF(CMAKE_BACKWARDS_COMPATIBILITY LESS 2.2)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(UsePkgConfig)
INCLUDE(CheckSymbolExists)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
# Turn on warnings
SET(CMAKE_C_FLAGS "-Wall -O2 ${CMAKE_C_FLAGS}")

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/src/include
	${CMAKE_BINARY_DIR}/src/include
	${CMAKE_BINARY_DIR}
)

# Colorize output, needs CMake > 2.3.4
OPTION(CMAKE_COLOR_MAKEFILE "Generate Makefiles with colored output" ON)

SET(CMAKE_INSTALL_PREFIX "" CACHE INTERNAL "")
SET(INCLUDE_INSTALL_DIR /usr/include CACHE STRING "Install location for header files.")
SET(SBIN_INSTALL_DIR /sbin CACHE STRING "Install location for system binaries.")
SET(BIN_INSTALL_DIR /bin CACHE STRING "Install location for binaries.")
SET(LIB_INSTALL_DIR /lib CACHE STRING "Install location for libraries.")
SET(SYSCONF_INSTALL_DIR /etc CACHE STRING "Install location for configuration files.")
SET(DATA_INSTALL_DIR /usr/share CACHE STRING "Install location for data files.")
SET(MAN_INSTALL_DIR ${DATA_INSTALL_DIR}/man CACHE STRING "Install location for manpages.")
MARK_AS_ADVANCED(INCLUDE_INSTALL_DIR)
MARK_AS_ADVANCED(SBIN_INSTALL_DIR)
MARK_AS_ADVANCED(BIN_INSTALL_DIR)
MARK_AS_ADVANCED(LIB_INSTALL_DIR)
MARK_AS_ADVANCED(SYSCONF_INSTALL_DIR)
MARK_AS_ADVANCED(DATA_INSTALL_DIR)
MARK_AS_ADVANCED(MAN_INSTALL_DIR)


SET(RUNLEVEL_DEFAULT runlevel/default CACHE STRING "Default runlevel.")
SET(RUNLEVEL_FAKE runlevel/fake-default CACHE STRING "Fake-mode runlevel.")
MARK_AS_ADVANCED(RUNLEVEL_DEFAULT)
MARK_AS_ADVANCED(RUNLEVEL_FAKE)

PKGCONFIG(dbus-1 DBUS_INCLUDE_DIR DBUS_LINK_DIR DBUS_LINK_FLAGS DBUS_CFLAGS)
SET(DBUS_DEFINITIONS ${DBUS_CFLAGS})
FIND_LIBRARY(DBUS_LIBS NAMES dbus-1 PATHS ${DBUS_LINK_DIR} /usr/lib /usr/local/lib)

SUBDIRS(src doc extras)

INSTALL_FILES(${INCLUDE_INSTALL_DIR}/initng FILES src/include/initng/paths.h)

OPTION(BUILD_ALSO "Build also plugin" ON)
OPTION(BUILD_SERVICE_FILE "Build service_file plugin" ON )
OPTION(BUILD_CHDIR "Build chdir plugin" ON)
OPTION(BUILD_CHROOT "Build chroot plugin" ON)
OPTION(BUILD_CONFLICT "Build conflict plugin" ON)
OPTION(BUILD_CPOUT "Build cpout plugin" ON)
OPTION(BUILD_CRITICAL "Build critical plugin" ON)
OPTION(BUILD_CTRLALTDEL "Build ctrlaltdel plugin" ON)
OPTION(BUILD_DAEMON_CLEAN "Build daemon_clean plugin" OFF)
OPTION(BUILD_DBUS_EVENT "Build dbus_event plugin" OFF)
OPTION(BUILD_DEBUG_COMMANDS "Build debug_commands plugin" ON)
OPTION(BUILD_TASK "Build task service type plugin" ON)
OPTION(BUILD_FMON "Build fmon plugin" ON)
OPTION(BUILD_FSTAT "Build fstat plugin" ON)
OPTION(BUILD_HISTORY "Build history plugin" OFF)
OPTION(BUILD_INITCTL "Build initctl plugin" ON)
OPTION(BUILD_INTERACTIVE "Build interactive plugin" ON)
OPTION(BUILD_LAST "Build last plugin" ON)
OPTION(BUILD_LIMIT "Build limitplugin" ON)
OPTION(BUILD_LOGFILE "Build logfile plugin" ON)
OPTION(BUILD_LOCKFILE "Build lockfile plugin" ON)
OPTION(BUILD_NETDEV "Build netdev plugin" ON)
OPTION(BUILD_IDLEPROBE "Build idleprobe plugin" ON)
OPTION(BUILD_NGC4 "Build ngc4 plugin" ON)
OPTION(BUILD_NGCS "Build ngcs plugin" OFF)
OPTION(BUILD_NGE "Build nge plugin" ON)
OPTION(BUILD_PAUSE "Build pause plugin" ON)
OPTION(BUILD_PROVIDE "Build provide plugin" ON)
OPTION(BUILD_RELOAD "Build reload plugin" ON)
OPTION(BUILD_RENICE "Build renice plugin" ON)
OPTION(BUILD_SIMPLE_LAUNCHER "Build simple_launcher plugin" ON)
OPTION(BUILD_STCMD "Build stcmd plugin" ON)
OPTION(BUILD_STDOUT "Build stdout plugin" ON)
OPTION(BUILD_SUID "Build suid plugin" ON)
OPTION(BUILD_SYNCRON "Build syncron plugin" ON)
OPTION(BUILD_SYSLOG "Build syslog plugin" ON)
OPTION(BUILD_SYSREQ "Build sysreq plugin" ON)
OPTION(BUILD_UNNEEDED "Build unneeded plugin" ON)
OPTION(BUILD_USPLASH "Build initng with usplash support" OFF)

OPTION(SELINUX_SUPPORT "SELinux support" OFF)
OPTION(INSTALL_AS_INIT "Install initng as complete replacement for SysVInit" OFF)
OPTION(DEBUG "Build with developer tools, turns on additional warnings and -Werror" OFF)
OPTION(CHECK_RO "Test if the / is mounted read-only before reboot/halt" ON)
OPTION(FORCE_NOCOLOR "Prevent cpout plugin from using colors" OFF)
OPTION(FORCE_POSIX_IFILES "Force POSIX compliance on ifiles" OFF)
OPTION(WITH_BUSYBOX "Hacks to make initng work nice with busybox distros" OFF)
OPTION(COUNT_ME "Send a simple http reqest to initng server, with initng version data to count initng users out there" OFF)

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

IF(COUNT_ME)
	EXEC_PROGRAM(${CMAKE_CURRENT_SOURCE_DIR}/count_me.sh)
ENDIF(COUNT_ME)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/include/initng/paths.h.cmake
	       ${CMAKE_CURRENT_BINARY_DIR}/src/include/initng/paths.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
