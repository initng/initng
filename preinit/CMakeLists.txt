SET(preinit_srcs
	main.c
)

IF(HAVE_SELINUX)
	SET(preinit_srcs ${preinit_srcs} selinux.c)
ENDIF(HAVE_SELINUX)

ADD_EXECUTABLE(preinit ${preinit_srcs})
TARGET_LINK_LIBRARIES(preinit libinitng)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} preinit)
