AUX_SOURCE_DIRECTORY(frontend           frontend_srcs)
AUX_SOURCE_DIRECTORY(active_db		active_db_srcs)
AUX_SOURCE_DIRECTORY(module		module_srcs)
AUX_SOURCE_DIRECTORY(event		event_srcs)
AUX_SOURCE_DIRECTORY(process_db		process_db_srcs)
AUX_SOURCE_DIRECTORY(service		service_srcs)
AUX_SOURCE_DIRECTORY(string		string_srcs)
AUX_SOURCE_DIRECTORY(toolbox		toolbox_srcs)
AUX_SOURCE_DIRECTORY(env		env_srcs)
AUX_SOURCE_DIRECTORY(active_state	active_state_srcs)
AUX_SOURCE_DIRECTORY(fork		fork_srcs)
AUX_SOURCE_DIRECTORY(signal		signal_srcs)
AUX_SOURCE_DIRECTORY(fd			fd_srcs)
AUX_SOURCE_DIRECTORY(common		common_srcs)
AUX_SOURCE_DIRECTORY(error		error_srcs)
AUX_SOURCE_DIRECTORY(command		command_srcs)
AUX_SOURCE_DIRECTORY(execute		execute_srcs)
AUX_SOURCE_DIRECTORY(handler		handler_srcs)
AUX_SOURCE_DIRECTORY(depend		depend_srcs)
AUX_SOURCE_DIRECTORY(interrupt		interrupt_srcs)
AUX_SOURCE_DIRECTORY(kill		kill_srcs)
AUX_SOURCE_DIRECTORY(static		static_srcs)
AUX_SOURCE_DIRECTORY(module_callers	module_callers_srcs)
AUX_SOURCE_DIRECTORY(io			io_srcs)
AUX_SOURCE_DIRECTORY(main		main_srcs)
AUX_SOURCE_DIRECTORY(data		data_srcs)
AUX_SOURCE_DIRECTORY(config		config_srcs)
AUX_SOURCE_DIRECTORY(hash		hash_srcs)
SET(libinitng_srcs
	${hash_srcs}
	${active_db_srcs}
	${module_srcs}
	${event_srcs}
	${process_db_srcs}
	${service_srcs}
	${string_srcs}
	${toolbox_srcs}
	${env_srcs}
	${active_state_srcs}
	${fork_srcs}
	${signal_srcs}
	${fd_srcs}
	${common_srcs}
	${error_srcs}
	${command_srcs}
	${execute_srcs}
	${handler_srcs}
	${depend_srcs}
	${interrupt_srcs}
	${kill_srcs}
	${static_srcs}
	${module_callers_srcs}
	${io_srcs}
	${main_srcs}
	${data_srcs}
	${config_srcs}
)

ADD_LIBRARY(libinitng STATIC ${libinitng_srcs})
SET_TARGET_PROPERTIES(libinitng PROPERTIES OUTPUT_NAME initng)

ADD_EXECUTABLE(initng ${frontend_srcs})
TARGET_LINK_LIBRARIES(initng
	-Wl,--whole-archive libinitng -Wl,--no-whole-archive
	dl
	${COREDUMPER_LIBS}
	${SELINUX_LIBS}
)

SET_TARGET_PROPERTIES(initng PROPERTIES
	ENABLE_EXPORTS TRUE
	LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/core.map"
)
INSTALL_TARGETS(${SBIN_INSTALL_DIR} initng)
