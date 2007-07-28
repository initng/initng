INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

SET(preinit_bin_srcs
	main.c
)

IF(HAVE_SELINUX AND NOT HAVE_SELINUX_NEW)
	SET(preinit_bin_srcs ${preinit_bin_srcs} oldselinux.c)
ENDIF(HAVE_SELINUX AND NOT HAVE_SELINUX_NEW)

ADD_EXECUTABLE(preinit_bin ${preinit_bin_srcs})
TARGET_LINK_LIBRARIES(preinit_bin initng)
SET_TARGET_PROPERTIES(preinit_bin PROPERTIES OUTPUT_NAME initng-preinit)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} preinit_bin)
