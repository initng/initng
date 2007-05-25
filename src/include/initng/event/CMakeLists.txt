SET(headers
	event.h
	types.h
	hook.h
	static_types.h
	all.h
)

INSTALL_FILES(${INCLUDE_INSTALL_DIR}/initng/event FILES ${headers})
