ADD_LIBRARY(cpout MODULE initng_colorprint_out.c)
TARGET_LINK_LIBRARIES(cpout initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng cpout)

