INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )

SET(initng_lib_srcs
	initng_active_db.c
	initng_active_state.c
	initng_common.c
	initng_control_command.c
	initng_depend.c
	initng_error.c
	initng_execute.c
	initng_env_variable.c
	initng_fd.c
	initng_fork.c
	initng_handler.c
	initng_interrupt.c
	initng_kill_handler.c
	initng_load_module.c
	initng_main.c
	initng_open_read_close.c
	initng_plugin_callers.c
	initng_process_db.c
	initng_service_data_types.c
	initng_static_event_types.c
	initng_service_types.c
	initng_signal.c
	initng_static_data_id.c
	initng_static_states.c
	initng_static_service_types.c
	initng_string_tools.c
	initng_struct_data.c
	initng_toolbox.c
	initng_event_types.c
	initng_event_hook.c
	initng_event.c
)

ADD_LIBRARY(initng SHARED ${initng_lib_srcs})
TARGET_LINK_LIBRARIES(initng dl ${COREDUMPER_LIBS} ${SELINUX_LIBS})
SET_TARGET_PROPERTIES(initng PROPERTIES VERSION 0.0.0 SOVERSION 0)
INSTALL_TARGETS(${LIB_INSTALL_DIR} initng)
