ADD_LIBRARY(provide MODULE initng_provide.c)
TARGET_LINK_LIBRARIES(provide initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng provide)

