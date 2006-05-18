MESSAGE("-- Creating links")
SET(TARGETS
	ngdc
	ngstart
	ngstop
	ngreboot
	nghalt
	ngzap
	ngrestart
	ngstatus
)

FOREACH(target ${TARGETS})
	EXEC_PROGRAM(ln $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin
		ARGS -sf ngc ${target})
ENDFOREACH(target)
