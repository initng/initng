INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

ADD_LIBRARY(interactive MODULE initng_interactive.c)
TARGET_LINK_LIBRARIES(interactive initng)
INSTALL_TARGETS(${LIB_INSTALL_DIR}/initng interactive)

