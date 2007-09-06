ADD_LIBRARY(conflict MODULE initng_conflict.c)
TARGET_LINK_LIBRARIES(conflict initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng conflict)

