ADD_LIBRARY(renice MODULE initng_renice.c)
TARGET_LINK_LIBRARIES(renice initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng renice)

