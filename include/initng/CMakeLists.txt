SUBDIRS(event config service module static)

SET(headers
	misc.h
	plugin.h
	system_states.h
	list.h
	active_db.h
	active_state.h
	common.h
	command.h
	depend.h
	error.h
	execute.h
	env.h
	fork.h
	handler.h
	hash.h
	interrupt.h
	is_state.h
	kill.h
	main.h
	io.h
	plugin_callers.h
	process_db.h
	signal.h
	string.h
	data.h
	toolbox.h
	msg.h
)

INSTALL_FILES(${INCLUDE_INSTALL_DIR}/initng FILES ${headers})
