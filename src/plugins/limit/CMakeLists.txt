ADD_LIBRARY(limit MODULE initng_limit.c)
TARGET_LINK_LIBRARIES(limit initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng limit)

