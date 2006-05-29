MESSAGE("-- Creating links")
SET(TARGETS
	ngdcs
)

IF(NOT BUILD_NGC4)
SET(TARGETS
	ngdcs
	ngc
	ngdc
	ngstart
	ngstop
	ngreboot
	nghalt
	ngzap
	ngrestart
	ngstatus
)
ENDIF(NOT BUILD_NGC4)

FOREACH(target ${TARGETS})
	EXEC_PROGRAM(ln $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin
		ARGS -sf ngcs ${target})
ENDFOREACH(target)
