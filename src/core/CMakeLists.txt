INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

SET(core_bin_srcs
	main.c
	options.c
)

ADD_EXECUTABLE(core_bin ${core_bin_srcs})
TARGET_LINK_LIBRARIES(core_bin initng)
SET_TARGET_PROPERTIES(core_bin PROPERTIES OUTPUT_NAME initng-core)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} core_bin)
