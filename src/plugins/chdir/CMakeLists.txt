ADD_LIBRARY(chdir MODULE initng_chdir.c)
TARGET_LINK_LIBRARIES(chdir initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng chdir)

