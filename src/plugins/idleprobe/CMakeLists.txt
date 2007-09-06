ADD_LIBRARY(idleprobe MODULE initng_idleprobe.c)
TARGET_LINK_LIBRARIES(idleprobe initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng idleprobe)

