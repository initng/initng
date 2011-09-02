/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <initng.h>

#include <stdio.h>
#include <stdlib.h>		/* free() exit() */
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};

/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTION DEFINES                  #
 * ############################################################################
 */
static int start_SERVICE(active_db_h * service_to_start);
static int stop_SERVICE(active_db_h * service);

/*
 * ############################################################################
 * #                        STATE HANDLERS FUNCTION DEFINES                   #
 * ############################################################################
 */
static void handle_SERVICE_WAITING_FOR_START_DEP(active_db_h * service);
static void handle_SERVICE_WAITING_FOR_STOP_DEP(active_db_h * service);

static void init_SERVICE_START_DEPS_MET(active_db_h * service);
static void init_SERVICE_STOP_DEPS_MET(active_db_h * service);
static void init_SERVICE_START_MARKED(active_db_h * service);
static void init_SERVICE_STOP_MARKED(active_db_h * service);

static void timeout_SERVICE_START_RUN(active_db_h * service);
static void init_SERVICE_START_RUN(active_db_h * service);
static void timeout_SERVICE_STOP_RUN(active_db_h * service);
static void init_SERVICE_STOP_RUN(active_db_h * service);

/*
 * ############################################################################
 * #                        PROCESS TYPE FUNCTION DEFINES                     #
 * ############################################################################
 */
static void handle_killed_start(active_db_h * killed_service,
				process_h * process);
static void handle_killed_stop(active_db_h * killed_service,
			       process_h * process);

/*
 * ############################################################################
 * #                     Official SERVICE_TYPE STRUCT                         #
 * ############################################################################
 */
stype_h TYPE_SERVICE = {
	.name = "service",
	.description = "The start code will run on start, and stop code when "
	    "stopping.",
	.hidden = FALSE,
	.start = &start_SERVICE,
	.stop = &stop_SERVICE,
	.restart = NULL
};

/*
 * ############################################################################
 * #                       PROCESS TYPES STRUCTS                              #
 * ############################################################################
 */
ptype_h T_START = {
	.name = "start",
	.kill_handler = &handle_killed_start
};

ptype_h T_STOP = {
	.name = "stop",
	.kill_handler = &handle_killed_stop
};

/*
 * ############################################################################
 * #                       SERVICE VARIABLES                                  #
 * ############################################################################
 */

#define DEFAULT_START_TIMEOUT 240
#define DEFAULT_STOP_TIMEOUT 60
s_entry START_TIMEOUT = {
	.name = "start_timeout",
	.description = "Let the start process run maximum this time.",
	.type = INT,
	.ot = &TYPE_SERVICE,
};

s_entry STOP_TIMEOUT = {
	.name = "stop_timeout",
	.description = "Let the stop process run maximum this time.",
	.type = INT,
	.ot = &TYPE_SERVICE,
};

s_entry NEVER_KILL = {
	.name = "never_kill",
	.description = "This service is to important to be killed by any "
	    "timeout!",
	.type = SET,
	.ot = &TYPE_SERVICE,
};

s_entry START_FAIL_OK = {
	.name = "start_fail_ok",
	.description = "Set this service to a succes, even if it returns bad "
	    "on start service.",
	.type = SET,
	.ot = &TYPE_SERVICE,
};

s_entry STOP_FAIL_OK = {
	.name = "stop_fail_ok",
	.description = "Set this service to stopped even if it returns bad "
	    "on stop service.",
	.type = SET,
	.ot = &TYPE_SERVICE,
};

/*
 * ############################################################################
 * #                         SERVICE STATES STRUCTS                           #
 * ############################################################################
 */

/*
 * When we want to start a service, it is first SERVICE_START_MARKED
 */
a_state_h SERVICE_START_MARKED = {
	.name = "SERVICE_START_MARKED",
	.description = "This service is marked for starting.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_SERVICE_START_MARKED,
	.alarm = NULL
};

/*
 * When we want to stop a SERVICE_DONE service, its marked SERVICE_STOP_MARKED
 */
a_state_h SERVICE_STOP_MARKED = {
	.name = "SERVICE_STOP_MARKED",
	.description = "This service is marked for stopping.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_SERVICE_STOP_MARKED,
	.alarm = NULL
};

/*
 * When a service is UP, it is marked as SERVICE_DONE
 */
a_state_h SERVICE_DONE = {
	.name = "SERVICE_DONE",
	.description = "The start code has successfully returned, the "
	    "service is marked as UP.",
	.is = IS_UP,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * When services needed by current one is starting, current service is set SERVICE_WAITING_FOR_START_DEP
 */
a_state_h SERVICE_WAITING_FOR_START_DEP = {
	.name = "SERVICE_WAITING_FOR_START_DEP",
	.description = "This service is waiting for all its depdencencies to "
	    "be met.",
	.is = IS_STARTING,
	.interrupt = &handle_SERVICE_WAITING_FOR_START_DEP,
	.init = NULL,
	.alarm = NULL
};

/*
 * When services needed to stop, before this is stopped is stopping, current service is set SERVICE_WAITING_FOR_STOP_DEP
 */
a_state_h SERVICE_WAITING_FOR_STOP_DEP = {
	.name = "SERVICE_WAITING_FOR_STOP_DEP",
	.description = "This service is wating for all services depending it "
	    "to stop, before stopping.",
	.is = IS_STOPPING,
	.interrupt = &handle_SERVICE_WAITING_FOR_STOP_DEP,
	.init = NULL,
	.alarm = NULL
};

/*
 * This state is set, when all services needed to start, is started.
 */
a_state_h SERVICE_START_DEPS_MET = {
	.name = "SERVICE_START_DEPS_MET",
	.description = "The dependencies for starting this service are met.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_SERVICE_START_DEPS_MET,
	.alarm = NULL
};

/*
 * This state is set, when all services needed top be stopped, before stopping, is stopped.
 */
a_state_h SERVICE_STOP_DEPS_MET = {
	.name = "SERVICE_STOP_DEPS_MET",
	.description = "The dependencies for stopping this services are met.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_SERVICE_STOP_DEPS_MET,
	.alarm = NULL
};

/*
 * This marks the services, as DOWN.
 */
a_state_h SERVICE_STOPPED = {
	.name = "SERVICE_STOPPED",
	.description = "The stop code has been returned, service is marked "
	    "as DOWN.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * This is the state, when the Start code is actually running.
 */
a_state_h SERVICE_START_RUN = {
	.name = "SERVICE_START_RUN",
	.description = "The start code is currently running on this service.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_SERVICE_START_RUN,
	.alarm = &timeout_SERVICE_START_RUN
};

/*
 * This is the state, when the Stop code is actually running.
 */
a_state_h SERVICE_STOP_RUN = {
	.name = "SERVICE_STOP_RUN",
	.description = "The service stop code is being run.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_SERVICE_STOP_RUN,
	.alarm = &timeout_SERVICE_STOP_RUN
};

/*
 * Generally FAILING states, if something goes wrong, some of these are set.
 */

a_state_h SERVICE_START_DEPS_FAILED = {
	.name = "SERVICE_START_DEPS_FAILED",
	.description = "One of the depdencencies this service requires has "
	    "failed, this service cant start.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_START_LAUNCH = {
	.name = "SERVICE_FAIL_START_LAUNCH",
	.description = "Service failed to launch start script/executable.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_START_NONEXIST = {
	.name = "SERVICE_FAIL_START_NONEXIST",
	.description = "There was no start code avaible for this service.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_START_RCODE = {
	.name = "SERVICE_FAIL_START_RCODE",
	.description = "The start code returned a non-zero value, this "
	    "usually meens that the service failed",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_START_SIGNAL = {
	.name = "SERVICE_FAIL_START_SIGNAL",
	.description = "The service start code segfaulted.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_START_FAILED_TIMEOUT = {
	.name = "SERVICE_START_FAILED_TIMEOUT",
	.description = "Time out running the start code.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_STOP_DEPS_FAILED = {
	.name = "SERVICE_STOP_DEPS_FAILED",
	.description = "Failed to stop one of the services depending on this "
	    "service, cannot stop this service",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_STOP_NONEXIST = {
	.name = "SERVICE_FAIL_STOP_NONEXIST",
	.description = "The stop code did not exist.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_STOP_RCODE = {
	.name = "SERVICE_FAIL_STOP_RCODE",
	.description = "The stop exec returned a non-zero code, this usually "
	    "means a failure.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_FAIL_STOP_SIGNAL = {
	.name = "SERVICE_FAIL_STOP_SIGNAL",
	.description = "The stop exec segfaulted.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_STOP_FAILED_TIMEOUT = {
	.name = "SERVICE_STOP_FAILED_TIMEOUT",
	.description = "Timeout running the stop code.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h SERVICE_UP_CHECK_FAILED = {
	.name = "SERVICE_UP_CHECK_FAILED",
	.description = "The checks that are done before the service can be "
	    "set to UP failed",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTIONS                         #
 * ############################################################################
 */

/* This are run, when initng wants to start a service */
static int start_SERVICE(active_db_h * service)
{
	/* if not stopped yet, reset DONE */
	if (IS_MARK(service, &SERVICE_WAITING_FOR_STOP_DEP)) {
		initng_common_mark_service(service, &SERVICE_DONE);
		return TRUE;
	}

	/* mark it WAITING_FOR_START_DEP and wait */
	if (!initng_common_mark_service(service, &SERVICE_START_MARKED)) {
		W_("mark_service SERVICE_START_MARKED failed for service "
		   "%s\n", service->name);
		return FALSE;
	}

	/* return happily */
	return TRUE;
}

/* This are run, when initng wants to stop a service */
static int stop_SERVICE(active_db_h * service)
{
	/* if not started yet, reset STOPPED */
	if (IS_MARK(service, &SERVICE_WAITING_FOR_START_DEP)) {
		initng_common_mark_service(service, &SERVICE_STOPPED);
		return TRUE;
	}

	/* set stopping */
	if (!initng_common_mark_service(service, &SERVICE_STOP_MARKED)) {
		W_("mark_service SERVICE_STOP_MARKED failed for service "
		   "%s.\n", service->name);
		return FALSE;
	}

	/* return happily */
	return TRUE;
}

/*
 * ############################################################################
 * #                          MODULE INITIATORS                               #
 * ############################################################################
 */

int module_init(void)
{
	initng_service_type_register(&TYPE_SERVICE);

	initng_process_db_ptype_register(&T_START);
	initng_process_db_ptype_register(&T_STOP);

	initng_active_state_register(&SERVICE_START_MARKED);
	initng_active_state_register(&SERVICE_STOP_MARKED);
	initng_active_state_register(&SERVICE_DONE);
	initng_active_state_register(&SERVICE_WAITING_FOR_START_DEP);
	initng_active_state_register(&SERVICE_WAITING_FOR_STOP_DEP);
	initng_active_state_register(&SERVICE_START_DEPS_MET);
	initng_active_state_register(&SERVICE_STOP_DEPS_MET);
	initng_active_state_register(&SERVICE_STOPPED);
	initng_active_state_register(&SERVICE_START_RUN);
	initng_active_state_register(&SERVICE_STOP_RUN);
	initng_active_state_register(&SERVICE_START_DEPS_FAILED);
	initng_active_state_register(&SERVICE_STOP_DEPS_FAILED);

	/* FAIL states, may got while starting */
	initng_active_state_register(&SERVICE_FAIL_START_LAUNCH);
	initng_active_state_register(&SERVICE_FAIL_START_NONEXIST);
	initng_active_state_register(&SERVICE_FAIL_START_RCODE);
	initng_active_state_register(&SERVICE_FAIL_START_SIGNAL);

	initng_active_state_register(&SERVICE_START_FAILED_TIMEOUT);
	initng_active_state_register(&SERVICE_STOP_FAILED_TIMEOUT);
	initng_active_state_register(&SERVICE_FAIL_STOP_NONEXIST);
	initng_active_state_register(&SERVICE_UP_CHECK_FAILED);
	initng_active_state_register(&SERVICE_FAIL_STOP_RCODE);
	initng_active_state_register(&SERVICE_FAIL_STOP_SIGNAL);

	initng_service_data_type_register(&START_TIMEOUT);
	initng_service_data_type_register(&STOP_TIMEOUT);
	initng_service_data_type_register(&NEVER_KILL);
	initng_service_data_type_register(&START_FAIL_OK);
	initng_service_data_type_register(&STOP_FAIL_OK);

	return TRUE;
}

void module_unload(void)
{
	initng_service_type_unregister(&TYPE_SERVICE);

	initng_process_db_ptype_unregister(&T_START);
	initng_process_db_ptype_unregister(&T_STOP);

	initng_active_state_unregister(&SERVICE_START_MARKED);
	initng_active_state_unregister(&SERVICE_STOP_MARKED);
	initng_active_state_unregister(&SERVICE_DONE);
	initng_active_state_unregister(&SERVICE_WAITING_FOR_START_DEP);
	initng_active_state_unregister(&SERVICE_WAITING_FOR_STOP_DEP);
	initng_active_state_unregister(&SERVICE_START_DEPS_MET);
	initng_active_state_unregister(&SERVICE_STOP_DEPS_MET);
	initng_active_state_unregister(&SERVICE_STOPPED);
	initng_active_state_unregister(&SERVICE_START_RUN);
	initng_active_state_unregister(&SERVICE_STOP_RUN);
	initng_active_state_unregister(&SERVICE_START_DEPS_FAILED);
	initng_active_state_unregister(&SERVICE_STOP_DEPS_FAILED);

	/* fail states may got while starting */
	initng_active_state_unregister(&SERVICE_FAIL_START_LAUNCH);
	initng_active_state_unregister(&SERVICE_FAIL_START_NONEXIST);
	initng_active_state_unregister(&SERVICE_FAIL_START_RCODE);
	initng_active_state_unregister(&SERVICE_FAIL_START_SIGNAL);

	initng_active_state_unregister(&SERVICE_START_FAILED_TIMEOUT);
	initng_active_state_unregister(&SERVICE_STOP_FAILED_TIMEOUT);
	initng_active_state_unregister(&SERVICE_UP_CHECK_FAILED);
	initng_active_state_unregister(&SERVICE_FAIL_STOP_NONEXIST);
	initng_active_state_unregister(&SERVICE_FAIL_STOP_RCODE);
	initng_active_state_unregister(&SERVICE_FAIL_STOP_SIGNAL);

	initng_service_data_type_unregister(&START_TIMEOUT);
	initng_service_data_type_unregister(&STOP_TIMEOUT);
	initng_service_data_type_unregister(&NEVER_KILL);
	initng_service_data_type_unregister(&START_FAIL_OK);
	initng_service_data_type_unregister(&STOP_FAIL_OK);
}

/*
 * ############################################################################
 * #                         STATE_FUNCTIONS                                  #
 * ############################################################################
 */

/*
 * Everything SERVICE_START_MARKED are gonna do, is to set it SERVICE_WAITING_FOR_START_DEP
 */
static void init_SERVICE_START_MARKED(active_db_h * service)
{
	/* Start our dependencies */
	if (initng_depend_start_deps(service) != TRUE) {
		initng_common_mark_service(service, &SERVICE_START_DEPS_FAILED);
		return;
	}

	/* set WAITING_FOR_START_DEP */
	initng_common_mark_service(service, &SERVICE_WAITING_FOR_START_DEP);
}

/*
 * Everything SERVICE_STOP_MARKED are gonna do, is to set it SERVICE_WAITING_FOR_STOP_DEP
 */
static void init_SERVICE_STOP_MARKED(active_db_h * service)
{
	/* Stopp all services dependeing on this service */
	if (initng_depend_stop_deps(service) != TRUE) {
		initng_common_mark_service(service, &SERVICE_STOP_DEPS_FAILED);
		return;
	}

	initng_common_mark_service(service, &SERVICE_WAITING_FOR_STOP_DEP);
}

static void handle_SERVICE_WAITING_FOR_START_DEP(active_db_h * service)
{
	assert(service);

	/*
	 * this checks depencncy.
	 * initng_depend_start_dep_met() will return:
	 * TRUE (dep is met)
	 * FALSE (dep is not met)
	 * FAIL (dep is failed)
	 */
	switch (initng_depend_start_dep_met(service, FALSE)) {
	case TRUE:
		break;

	case FAIL:
		initng_common_mark_service(service, &SERVICE_START_DEPS_FAILED);
		return;

	default:
		/* return and hope that this handler will be called
		 * again. */
		return;
	}

	/* if system is shutting down, don't start anything */
	if (g.sys_state != STATE_STARTING && g.sys_state != STATE_UP) {
		D_("Can't start service, when system status is: %i !\n",
		   g.sys_state);
		initng_common_mark_service(service, &SERVICE_STOPPED);
		return;
	}

	/* set status to START_DEP_MET */
	initng_common_mark_service(service, &SERVICE_START_DEPS_MET);
}

static void handle_SERVICE_WAITING_FOR_STOP_DEP(active_db_h * service)
{
	assert(service);

	/* check with other plug-ins, if it is ok to stop this service now */
	if (initng_depend_stop_dep_met(service, FALSE) != TRUE)
		return;

	/* ok, stopping deps are met */
	initng_common_mark_service(service, &SERVICE_STOP_DEPS_MET);
}

static void init_SERVICE_START_DEPS_MET(active_db_h * service)
{
	if (!initng_common_mark_service(service, &SERVICE_START_RUN))
		return;

	/* F I N A L L Y   S T A R T */
	switch (initng_execute_launch(service, &T_START, NULL)) {
	case FALSE:
		F_("Did not find a start entry for %s to run!\n", service->name);
		initng_common_mark_service(service,
					   &SERVICE_FAIL_START_NONEXIST);
		return;

	case FAIL:
		F_("Service %s, could not launch start, did not find "
		   "any to launch!\n", service->name);
		initng_common_mark_service(service, &SERVICE_FAIL_START_LAUNCH);
		return;

	default:
		return;
	}
}

static void init_SERVICE_STOP_DEPS_MET(active_db_h * service)
{
	/* mark this service as STOPPING */
	if (!initng_common_mark_service(service, &SERVICE_STOP_RUN))
		return;

	/* launch stop service */
	switch (initng_execute_launch(service, &T_STOP, NULL)) {
	case FAIL:
		F_("  --  (%s): fail launch stop!\n", service->name);
		initng_common_mark_service(service,
					   &SERVICE_FAIL_STOP_NONEXIST);
		return;

	case FALSE:
		/* there exists no stop process */
		initng_common_mark_service(service, &SERVICE_STOPPED);
		return;

	default:
		return;
	}

}

/*
 * When a service got the state SERVICE_START_RUN
 * this function will set the timeout, so the alarm
 * function will be called after a while.
 */
static void init_SERVICE_START_RUN(active_db_h * service)
{
	int timeout;

	D_("Service %s, run init_SERVICE_START_RUN\n", service->name);

	/* if NEVER_KILL is set, dont bother */
	if (is(&NEVER_KILL, service))
		return;

	/* get the timeout */
	timeout = get_int(&START_TIMEOUT, service);

	/* if not set, use the default one */
	if (!timeout)
		timeout = DEFAULT_START_TIMEOUT;

	/* set state alarm */
	initng_handler_set_alarm(service, timeout);
}

/*
 * When a service got the state SERVICE_STOP_RUN
 * this function will set the timeout, so the alarm
 * function will be called after a while.
 */
static void init_SERVICE_STOP_RUN(active_db_h * service)
{
	int timeout;

	D_("Service %s, run init_SERVICE_STOP_RUN\n", service->name);

	/* if NEVER_KILL is set, dont bother */
	if (is(&NEVER_KILL, service))
		return;

	/* get the timeout */
	timeout = get_int(&STOP_TIMEOUT, service);

	/* if not set, use the default timeout */
	if (!timeout)
		timeout = DEFAULT_STOP_TIMEOUT;

	/* set state alarm */
	initng_handler_set_alarm(service, timeout);
}

static void timeout_SERVICE_START_RUN(active_db_h * service)
{
	process_h *process = NULL;

	/* if NEVER_KILL is set, dont bother */
	if (is(&NEVER_KILL, service))
		return;

	W_("Timeout for start process, service %s.  Killing this process "
	   "now.\n", service->name);

	/* get the process */
	if (!(process = initng_process_db_get(&T_START, service))) {
		F_("Did not find the T_START process!\n");
		return;
	}

	/* if the process does not exist on the system anymore, run the killd
	 * handler. */
	if (process->pid <= 0 || (kill(process->pid, 0) && errno == ESRCH)) {
		W_("The start process have dissapeared from the system "
		   "without notice, running start kill handler.\n");
		handle_killed_start(service, process);
		return;
	}

	/* send the process the SIGKILL signal */
	kill(process->pid, SIGKILL);

	/* set service state, to a failure state */
	initng_common_mark_service(service, &SERVICE_START_FAILED_TIMEOUT);
}

static void timeout_SERVICE_STOP_RUN(active_db_h * service)
{
	process_h *process = NULL;

	/* if NEVER_KILL is set, dont bother */
	if (is(&NEVER_KILL, service))
		return;

	W_("Timeout for stop process, service %s. Killing this process now.\n",
	   service->name);

	/* get the process */
	if (!(process = initng_process_db_get(&T_STOP, service))) {
		F_("Did not find the T_STOP process!\n");
		return;
	}

	/* if the process does not exist on the system anymore, run the killd
	 * handler. */
	if (process->pid <= 0 || (kill(process->pid, 0) && errno == ESRCH)) {
		W_("The stop process have dissapeared from the system "
		   "without notice, running stop kill handler.\n");
		handle_killed_stop(service, process);
		return;
	}

	/* send the process the SIGKILL signal */
	kill(process->pid, SIGKILL);

	/* set service state, to a failure state */
	initng_common_mark_service(service, &SERVICE_STOP_FAILED_TIMEOUT);
}

/*
 * ############################################################################
 * #                         KILL HANDLER FUNCTIONS                            #
 * ############################################################################
 */

/*
 * handle_killed_start(), description:
 *
 * A process dies, and kill_handler walks the active_db looking for a match
 * on that pid, if it founds a match for a active->start_process, this
 * function is called, with a pointer to that active_service to
 * handle the situation.
 */
static void handle_killed_start(active_db_h * service, process_h * process)
{
	assert(service);
	assert(service->name);
	assert(service->current_state);
	assert(service->current_state->name);
	assert(process);

	D_("handle_killed_start(%s): initial status: \"%s\".\n",
	   service->name, service->current_state->name);

	/* free this process what ever happends */
	int rcode = process->r_code;	/* save rcode */
	initng_process_db_free(process);

	/*
	 * If this exited after a timeout, stay failed
	 */
	if (IS_MARK(service, &SERVICE_START_FAILED_TIMEOUT))
		return;

	/*
	 * If service is stopping, ignore this signal
	 */
	if (!IS_MARK(service, &SERVICE_START_RUN)) {
		F_("Start exited!, and service is not marked starting!\n"
		   "Was this one launched manually by ngc --run ??");
		return;
	}

	/* check so it did not segfault */

	if (WTERMSIG(rcode) == 11) {
		F_("Service %s process start SEGFAULTED!\n", service->name);
		initng_common_mark_service(service, &SERVICE_FAIL_START_SIGNAL);
		return;
	}

	/*
	 * Make sure r_code don't signal error (can be override by UP_ON_FAILURE.
	 */

	/* if rcode > 0 */
	if (WEXITSTATUS(rcode) > 0 && !is(&START_FAIL_OK, service)) {
		F_(" start %s, Returned with TERMSIG %i rcode %i.\n",
		   service->name, WTERMSIG(rcode), WEXITSTATUS(rcode));
		initng_common_mark_service(service, &SERVICE_FAIL_START_RCODE);
		return;
	}

	/* TODO, create state WAITING_FOR_UP_CHECK */

	/* make the final up check */
	if (initng_depend_up_check(service) == FAIL) {
		initng_common_mark_service(service, &SERVICE_UP_CHECK_FAILED);
		return;
	}

	/* OK! now service is DONE! */
	initng_common_mark_service(service, &SERVICE_DONE);
}

/* to do when a stop action dies */
static void handle_killed_stop(active_db_h * service, process_h * process)
{
	assert(service);
	assert(service->name);
	assert(service->current_state);
	assert(service->current_state->name);
	assert(process);

	D_("(%s);\n", service->name);

	/* Free the process what ever happens below */
	int rcode = process->r_code;
	initng_process_db_free(process);

	/*
	 * If this exited after a timeout, stay failed
	 */
	if (IS_MARK(service, &SERVICE_STOP_FAILED_TIMEOUT))
		return;

	/* make sure its a STOP_RUN state */
	if (!IS_MARK(service, &SERVICE_STOP_RUN)) {
		F_("stop service died, but service is not status STOPPING!\n");
		return;
	}

	/* check with SIGSEGV (11) */
	if (WTERMSIG(rcode) == 11) {
		F_(" service %s stop process SEGFAUTED!\n", service->name);

		/* mark service stopped */
		initng_common_mark_service(service, &SERVICE_FAIL_STOP_SIGNAL);
		return;
	}

	/*
	 * If the return code (for example "exit 1", in a bash script)
	 * from the program, is bigger than 0, this commonly signalize
	 * that something got wrong, print this as an error msg on screen
	 */
	if (WEXITSTATUS(rcode) > 0 && !is(&STOP_FAIL_OK, service)) {
		F_(" stop %s, Returned with exit %i.\n", service->name,
		   WEXITSTATUS(rcode));

		/* mark service stopped */
		initng_common_mark_service(service, &SERVICE_FAIL_STOP_RCODE);
		return;
	}

	/* mark service stopped */
	initng_common_mark_service(service, &SERVICE_STOPPED);
}
