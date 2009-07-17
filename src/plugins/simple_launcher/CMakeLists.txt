ADD_LIBRARY(simple_launcher MODULE initng_simple_launcher.c)
TARGET_LINK_LIBRARIES(simple_launcher initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng simple_launcher)
SET_TARGET_PROPERTIES(simple_launcher PROPERTIES LINK_FLAGS ${MOD_LDFLAGS} COMPILE_FLAGS ${MOD_CFLAGS})
