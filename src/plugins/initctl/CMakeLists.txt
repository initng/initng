ADD_LIBRARY(initctl MODULE initng_initctl.c)
TARGET_LINK_LIBRARIES(initctl initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng initctl)

