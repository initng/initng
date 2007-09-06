ADD_LIBRARY(lockfile MODULE initng_lockfile.c)
TARGET_LINK_LIBRARIES(lockfile initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng lockfile)

