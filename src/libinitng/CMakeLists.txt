INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR} )
SET(initng_lib_srcs
	active_db/alloc.c
	active_db/find.c
	active_db/register.c
	active_db/utils.c
	module/depend.c
	module/load.c
	module/unload.c
	event/hook.c
	event/send.c
	event/type.c
	process_db/alloc.c
	process_db/find.c
	process_db/pipe.c
	process_db/type.c
	service/data_type.c
	service/type.c
	string/dup.c
	string/mprintf.c
	string/replace.c
	string/match.c
	string/path.c
	string/split_delim.c
	toolbox/alloc.c
	toolbox/set_proc_title.c
	env/alloc.c
	env/match.c

	initng_active_state.c
	initng_common.c
	initng_control_command.c
	initng_depend.c
	initng_error.c
	initng_execute.c
	initng_fd.c
	initng_fork.c
	initng_handler.c
	initng_interrupt.c
	initng_kill_handler.c
	initng_open_read_close.c
	initng_plugin_callers.c
	initng_signal.c
	initng_struct_data.c
)

ADD_LIBRARY(initng SHARED ${initng_lib_srcs})
TARGET_LINK_LIBRARIES(initng dl ${COREDUMPER_LIBS} ${SELINUX_LIBS})
SET_TARGET_PROPERTIES(initng PROPERTIES VERSION 0.0.0 SOVERSION 0)
INSTALL_TARGETS(${LIB_INSTALL_DIR} initng)
