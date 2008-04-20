PROJECT(libinitng C)

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
SET(CMAKE_C_FLAGS "-std=gnu99 -Wall -O2 ${CMAKE_C_FLAGS}")

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}/include
	${CMAKE_BINARY_DIR}
)

# Colorize output, needs CMake > 2.3.4
OPTION(CMAKE_COLOR_MAKEFILE "Generate Makefiles with colored output" ON)

SET(CMAKE_INSTALL_PREFIX "" CACHE INTERNAL "")
SET(INCLUDE_INSTALL_DIR /usr/include CACHE STRING "Install location for header files.")
SET(LIB_INSTALL_DIR /lib CACHE STRING "Install location for libraries.")
SET(DATA_INSTALL_DIR /usr/share CACHE STRING "Install location for data files.")
SET(MAN_INSTALL_DIR ${DATA_INSTALL_DIR}/man CACHE STRING "Install location for manpages.")
SET(DOC_INSTALL_DIR ${DATA_INSTALL_DIR}/doc CACHE STRING "Install location for documentation.")
MARK_AS_ADVANCED(INCLUDE_INSTALL_DIR)
MARK_AS_ADVANCED(LIB_INSTALL_DIR)
MARK_AS_ADVANCED(MAN_INSTALL_DIR)

PKGCONFIG(dbus-1 DBUS_INCLUDE_DIR DBUS_LINK_DIR DBUS_LINK_FLAGS DBUS_CFLAGS)
SET(DBUS_DEFINITIONS ${DBUS_CFLAGS})
FIND_LIBRARY(DBUS_LIBS NAMES dbus-1 PATHS ${DBUS_LINK_DIR} /usr/lib /usr/local/lib)

SUBDIRS(src include doc)

INSTALL_FILES(${INCLUDE_INSTALL_DIR}/initng FILES include/initng/paths.h)

OPTION(DEBUG "Build with developer tools, turns on additional warnings and -Werror" OFF)
OPTION(CHECK_RO "Test if the / is mounted read-only before reboot/halt" ON)

IF(DEBUG)
	CHECK_INCLUDE_FILES(google/coredumper.h HAVE_COREDUMPER_H)
	CHECK_LIBRARY_EXISTS(coredumper WriteCoreDump "" HAVE_COREDUMPER)

	IF(HAVE_COREDUMPER AND HAVE_COREDUMPER_H)
        	ADD_DEFINITIONS(-DHAVE_COREDUMPER)
		SET(COREDUMPER_LIBS coredumper)
	ENDIF(HAVE_COREDUMPER AND HAVE_COREDUMPER_H)

	SET(CMAKE_C_FLAGS "-DDEBUG -g -Werror -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wimplicit -Wredundant-decls -Wnested-externs -Wwrite-strings -Wsign-compare -Winline -Wswitch -Wreturn-type -Wparentheses -Wmissing-braces -Wformat -Wformat-nonliteral -Wformat-security -Wsequence-point -Wundef -Wunused -Wcomment ${CMAKE_C_FLAGS}")
ENDIF(DEBUG)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/initng/paths.h.cmake
	       ${CMAKE_CURRENT_BINARY_DIR}/include/initng/paths.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
