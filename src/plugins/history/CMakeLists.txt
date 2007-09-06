ADD_LIBRARY(history MODULE initng_history.c)
TARGET_LINK_LIBRARIES(history initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng history)

