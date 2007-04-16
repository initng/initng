INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

SET(initng_bin_srcs
	main.c
	global.c
	parse_args.c
	static_event_types.c
)

ADD_EXECUTABLE(initng_bin ${initng_bin_srcs})
TARGET_LINK_LIBRARIES(initng_bin initng)
SET_TARGET_PROPERTIES(initng_bin PROPERTIES OUTPUT_NAME initng)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} initng_bin)
