ADD_LIBRARY(sysreq MODULE initng_sysreq.c)
TARGET_LINK_LIBRARIES(sysreq initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng sysreq)

