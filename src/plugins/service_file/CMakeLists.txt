SUBDIRS(wrappers)

ADD_LIBRARY(service_file MODULE initng_service_file.c)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng service_file)

ADD_EXECUTABLE(bp bp.c)
SET_TARGET_PROPERTIES(bp PROPERTIES POST_INSTALL_SCRIPT
	${CMAKE_CURRENT_BINARY_DIR}/create_links.cmake)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/ibin bp)

ADD_EXECUTABLE(runiscript runiscript.c)
TARGET_LINK_LIBRARIES(runiscript initng)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} runiscript)

CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/create_links.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/create_links.cmake"
	IMMEDIATE @ONLY)
