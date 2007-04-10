INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

ADD_LIBRARY(critical MODULE initng_critical.c active_db_print.c)
TARGET_LINK_LIBRARIES(critical initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng critical)

