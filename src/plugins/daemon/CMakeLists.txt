ADD_LIBRARY(daemon MODULE initng_daemon.c)
TARGET_LINK_LIBRARIES(daemon initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng daemon)

