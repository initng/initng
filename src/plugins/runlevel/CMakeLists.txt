ADD_LIBRARY(runlevel MODULE initng_runlevel.c)
TARGET_LINK_LIBRARIES(runlevel initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng runlevel)

