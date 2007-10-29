SET(launcher_bin_srcs
	main.c
)

IF(HAVE_SELINUX)
	SET(launcher_bin_srcs ${launcher_bin_srcs} selinux.c)
ENDIF(HAVE_SELINUX)

ADD_EXECUTABLE(launcher_bin ${launcher_bin_srcs})
TARGET_LINK_LIBRARIES(launcher_bin initng)
SET_TARGET_PROPERTIES(launcher_bin PROPERTIES OUTPUT_NAME initng)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} launcher_bin)
