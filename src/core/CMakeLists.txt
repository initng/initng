INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

SET(initng_bin_srcs
	main.c
	initng_main.c
	initng_global.c
	initng_parse_args.c
	initng_static_event_types.c
	initng_static_states.c
	initng_static_service_types.c
	initng_static_data_id.c
)

ADD_EXECUTABLE(initng_bin ${initng_bin_srcs})
TARGET_LINK_LIBRARIES(initng_bin initng)
SET_TARGET_PROPERTIES(initng_bin PROPERTIES OUTPUT_NAME initng)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} initng_bin)
