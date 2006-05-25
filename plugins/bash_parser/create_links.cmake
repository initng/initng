MESSAGE("-- Creating links")
SET(TARGETS
	iabort
	idone
	iget
	iregister
	iset
)


FOREACH(target ${TARGETS})
	EXEC_PROGRAM(ln $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/ibin
		ARGS -sf bp ${target})
ENDFOREACH(target)
