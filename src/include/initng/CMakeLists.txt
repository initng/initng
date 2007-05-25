SUBDIRS(event config service)

SET(headers
	misc.h
	module.h
	plugin.h
	system_states.h
	list.h
	active_db.h
	active_state.h
	common.h
	control_command.h
	depend.h
	error.h
	execute.h
	env.h
	fd.h
	fork.h
	handler.h
	interrupt.h
	is_state.h
	kill.h
	load_module.h
	main.h
	io.h
	plugin_callers.h
	process_db.h
	signal.h
	string.h
	data.h
	toolbox.h
	msg.h
	event.h
)

INSTALL_FILES(${INCLUDE_INSTALL_DIR}/initng FILES ${headers})
