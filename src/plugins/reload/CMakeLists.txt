ADD_LIBRARY(reload MODULE initng_reload.c)
TARGET_LINK_LIBRARIES(reload initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng reload)

