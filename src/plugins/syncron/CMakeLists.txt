ADD_LIBRARY(syncron MODULE initng_syncron.c)
TARGET_LINK_LIBRARIES(syncron initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng syncron)

