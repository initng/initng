ADD_LIBRARY(chroot MODULE initng_chroot.c)
TARGET_LINK_LIBRARIES(chroot initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng chroot)

