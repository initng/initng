ADD_LIBRARY(syslog MODULE initng_syslog.c)
TARGET_LINK_LIBRARIES(syslog initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng syslog)

