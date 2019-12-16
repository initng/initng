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

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>		/* isdigit */

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
 * #                            CONSTANT DEFINES                              #
 * ############################################################################
 */

/*
 * timeout waiting for a pidfile to be created
 * after the daemon returns happily.
 */
#define PID_TIMEOUT 60

/*
 * Rate limit on missing pidfile warnings
 */
#define PID_WARN_RATE 2

/*
 * seconds that must pass between respawns
 */
#define DEFAULT_RESPAWN_RATE 10

/*
 * Seconds to sleep, before a daemon is respawned.
 */
#define DEFAULT_RESPAWN_PAUSE 1

/*
 * Seconds a process have to exit after term signal sent, before kill is sent.
 */
#define DEFAULT_TERM_TIMEOUT 30

/*
 * Seconds a process have to exit after kill signal sent before we try and kill it again.
 */
#define DEFAULT_KILL_TIMEOUT 2

/*
 * ############################################################################
 * #                         LOCAL FUNCTION DEFINES                           #
 * ############################################################################
 */

static void kill_daemon(active_db_h * service, int sig);
static void clear_pidfile(active_db_h * s);
static pid_t get_pidof(active_db_h * s);
static pid_t get_pidfile(active_db_h * s);
static int check_respawn(active_db_h * service);
static int try_get_pid(active_db_h * s);

/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTION DEFINES                  #
 * ############################################################################
 */
static int start_DAEMON(active_db_h * daemon_to_start);
static int stop_DAEMON(active_db_h * daemon);

/*
 * ############################################################################
 * #                        STATE HANDLERS FUNCTION DEFINES                   #
 * ############################################################################
 */
static void handle_DAEMON_WAITING_FOR_START_DEP(active_db_h * daemon);
static void handle_DAEMON_WAITING_FOR_STOP_DEP(active_db_h * daemon);

static void init_DAEMON_START_DEPS_MET(active_db_h * daemon);
static void init_DAEMON_STOP_DEPS_MET(active_db_h * daemon);
static void init_DAEMON_START_MARKED(active_db_h * daemon);
static void init_DAEMON_STOP_MARKED(active_db_h * daemon);

static void init_DAEMON_WAIT_FOR_PID_FILE(active_db_h * daemon);
static void timeout_DAEMON_WAIT_FOR_PID_FILE(active_db_h * daemon);
static void init_DAEMON_WAIT_RESP_TOUT(active_db_h * service);
static void timeout_DAEMON_WAIT_RESP_TOUT(active_db_h * service);
static void init_DAEMON_KILL(active_db_h * daemon);
static void timeout_DAEMON_KILL(active_db_h * daemon);
static void init_DAEMON_TERM(active_db_h * daemon);
static void timeout_DAEMON_TERM(active_db_h * daemon);

static void check_DAEMON_UP(active_db_h * service);

/*
 * ############################################################################
 * #                        PROCESS TYPE FUNCTION DEFINES                     #
 * ############################################################################
 */
static void handle_killed_daemon(active_db_h * killed_daemon,
				 process_h * process);

/*
 * ############################################################################
 * #                     Official DAEMON_TYPE STRUCT                         #
 * ############################################################################
 */
stype_h TYPE_DAEMON = {
	.name = "daemon",
	.description = "A daemon is a process running in background.",
	.hidden = FALSE,
	.start = &start_DAEMON,
	.stop = &stop_DAEMON,
	.restart = NULL
};

/*
 * ############################################################################
 * #                       PROCESS TYPES STRUCTS                              #
 * ############################################################################
 */
ptype_h T_DAEMON = { "daemon", &handle_killed_daemon };
ptype_h T_KILL = { "kill", NULL };

/*
 * ############################################################################
 * #                       DAEMON VARIABLES                                   #
 * ############################################################################
 */

/*
 * The name of the process, initng should probe.
 */
s_entry PIDOF = {
	.name = "pid_of",
	.description = "When daemon exits, initng will look for a process "
	    "with this name, and set daemon pid no to that pid.",
	.type = STRING,
	.ot = &TYPE_DAEMON,
};

/*
 * The filename/path of a pidfile that initng can fetch pid no
 */
s_entry PIDFILE = {
	.name = "pid_file",
	.description = "When daemon exits, initng will get pid of daemon "
	    "from this file.",
	.type = STRINGS,
	.ot = &TYPE_DAEMON,
};

/*
 * If a daemon forks, and exit, but a copy is running, set the forks so we know this.
 */
s_entry FORKS = {
	.name = "forks",
	.description = "Does the daemon fork?",
	.type = SET,
	.ot = &TYPE_DAEMON,
};

/*
 * Is set if daemon should be restarted, when it dies.
 */
s_entry RESPAWN = {
	.name = "respawn",
	.description = "If this is set and daemon dies, it will be "
	    "restarted.",
	.type = SET,
	.ot = &TYPE_DAEMON,
};

/*
 * When respwaning, wait this many seconds before restarting the daemon.
 */
s_entry RESPAWN_PAUSE = {
	.name = "respawn_pause",
	.description = "Wait this number of seconds before respawning.",
	.type = INT,
	.ot = &TYPE_DAEMON,
};

/*
 * Minimum respawn rate, Seconds to pass sience last respawn.
 */
s_entry RESPAWN_RATE = {
	.name = "respawn_rate",
	.description = "The minimum of seconds that this daemon must be up, "
	    "to be respawned when it dies.",
	.type = INT,
	.ot = &TYPE_DAEMON,
};

/*
 * TERM timeout, when killing the daemon, wait this many seconds, before
 * Sending the KILL signal.
 */
s_entry TERM_TIMEOUT = {
	.name = "term_timeout",
	.description = "Wait this many seconds before KILL daemon, after a "
	    "TERM.",
	.type = INT,
	.ot = &TYPE_DAEMON,
};

/* A return code > 0 is ok */
s_entry DAEMON_FAIL_OK = {
	.name = "daemon_fail_ok",
	.description = "If daemon returns a positive return code, this is "
	    "till ok",
	.type = SET,
	.ot = &TYPE_DAEMON,
};

/* A return code > 0 is ok, but only if it's stopping */
s_entry DAEMON_STOPS_BADLY = {
	.name = "daemon_stops_badly",
	.description = "If the daemon is stopping and returns a positive "
	    "return code, this is OK",
	.type = SET,
	.ot = &TYPE_DAEMON,
};

/*
 * Down here, internal variables, that we use, but cant be set in the .i file.
 */

/* Last time respawning, so it wont respawn to mutch */
s_entry INTERNAL_LAST_RESPAWN = {
	.name = "internal_last_respawn",
	.type = INT,
	.ot = &TYPE_DAEMON,
	.description = NULL
};

/*
 * ############################################################################
 * #                         DAEMON STATES STRUCTS                            #
 * ############################################################################
 */

/*
 * When we want to start a daemon, it is first DAEMON_START_MARKED
 */
a_state_h DAEMON_START_MARKED = {
	.name = "DAEMON_START_MARKED",
	.description = "The daemon is marked for starting",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_DAEMON_START_MARKED,
	.alarm = NULL
};

/*
 * When we want to stop a DAEMON_RUNNING daemon, its marked SERVICE_STOP_MARKED
 */
a_state_h DAEMON_STOP_MARKED = {
	.name = "DAEMON_STOP_MARKED",
	.description = "The daemon is marked to be stopped.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_DAEMON_STOP_MARKED,
	.alarm = NULL
};

/*
 * When a daemon is UP, it is marked as SERVICE_DONE
 */
a_state_h DAEMON_RUNNING = {
	.name = "DAEMON_RUNNING",
	.description = "This daemon is running.",
	.is = IS_UP,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * When daemons needed by current one is starting, current daemon is set DAEMON_WAITING_FOR_START_DEP
 */
a_state_h DAEMON_WAITING_FOR_START_DEP = {
	.name = "DAEMON_WAITING_FOR_START_DEP",
	.description = "Waiting for dependencies before starting this "
	    "daemon",
	.is = IS_STARTING,
	.interrupt = &handle_DAEMON_WAITING_FOR_START_DEP,
	.init = NULL,
	.alarm = NULL
};

/*
 * When daemons needed to stop, before this is stopped is stopping, current daemon is set DAEMON_WAITING_FOR_STOP_DEP
 */
a_state_h DAEMON_WAITING_FOR_STOP_DEP = {
	.name = "DAEMON_WAITING_FOR_STOP_DEP",
	.description = "Waiting for dependencies to stop this daemon.",
	.is = IS_STOPPING,
	.interrupt = &handle_DAEMON_WAITING_FOR_STOP_DEP,
	.init = NULL,
	.alarm = NULL
};

/*
 * This state is set, when all daemons needed to start, is started.
 */
a_state_h DAEMON_START_DEPS_MET = {
	.name = "DAEMON_START_DEPS_MET",
	.description = "The dependencies for starting this daemon are met.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_DAEMON_START_DEPS_MET,
	.alarm = NULL
};

/*
 * This state is set, when all daemons needed top be stopped, before stopping, is stopped.
 */
a_state_h DAEMON_STOP_DEPS_MET = {
	.name = "DAEMON_STOP_DEPS_MET",
	.description = "The dependencies for stopping this daemon are met.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_DAEMON_STOP_DEPS_MET,
	.alarm = NULL
};

/*
 * This is the state on the daemon, when it are being killed.
 */
a_state_h DAEMON_KILL = {
	.name = "DAEMON_KILL",
	.description = "The code for killing this daemon is running.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_DAEMON_KILL,
	.alarm = &timeout_DAEMON_KILL
};

a_state_h DAEMON_TERM = {
	.name = "DAEMON_TERM",
	.description = "This daemon is terminating.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_DAEMON_TERM,
	.alarm = &timeout_DAEMON_TERM
};

/*
 * This marks the daemons, as DOWN.
 */
a_state_h DAEMON_STOPPED = {
	.name = "DAEMON_STOPPED",
	.description = "This daemon has been stopped.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * This is the state, when the launch is actually run.
 */
a_state_h DAEMON_LAUNCH = {
	.name = "DAEMON_LAUNCH",
	.description = "This daemon is launching.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * In this state, the fork has return, and initng is waiting for a pidfile to appeare.
 */
a_state_h DAEMON_WAIT_FOR_PID_FILE = {
	.name = "DAEMON_WAIT_FOR_PID_FILE",
	.description = "Initng is waiting for the daemon to set a pidfile.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_DAEMON_WAIT_FOR_PID_FILE,
	.alarm = &timeout_DAEMON_WAIT_FOR_PID_FILE
};

/*
 * This state, is when a daemon have been stopped, and its waiting for respawn timeout.
 */
a_state_h DAEMON_WAIT_RESP_TOUT = {
	.name = "WAIT_FOR_RESPAWN_TIMEOUT",
	.description = "This daemon is waiting a bit, before the daemon is "
	    "respawn.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_DAEMON_WAIT_RESP_TOUT,
	.alarm = &timeout_DAEMON_WAIT_RESP_TOUT
};

/*
 * Generally FAILING states, if something goes wrong, some of these are set.
 */
a_state_h DAEMON_START_DEPS_FAILED = {
	.name = "DAEMON_START_DEPS_FAILED",
	.description = "One of the dependencies for starting this daemon is "
	    "not met",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_STOP_DEPS_FAILED = {
	.name = "DAEMON_STOP_DEPS_FAILED",
	.description = "One of the dependencies for stopping this daemon is "
	    "not met.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_FAIL_START_RCODE = {
	.name = "DAEMON_FAIL_START_RCODE",
	.description = "The daemon exited with a positive return code.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_FAIL_START_SIGNAL = {
	.name = "DAEMON_FAIL_START_SIGNAL",
	.description = "The daemon segfaulted.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_FAIL_START_LAUNCH = {
	.name = "DAEMON_FAIL_START_LAUNCH",
	.description = "The daemon failed to launch, probably not found.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_FAIL_START_TIMEOUT_PIDFILE = {
	.name = "DAEMON_FAIL_START_TIMEOUT_PIDFILE",
	.description = "Initng timed out waiting for the daemon to set a "
	    "pidfile.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_WAITING_FOR_UP_CHECK = {
	.name = "DAEMON_WAITING_FOR_UP_CHECK",
	.description = "Waiting for the daemon reporting up. ",
	.is = IS_STARTING,
	.interrupt = &check_DAEMON_UP,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_FAIL_START_NONEXIST = {
	.name = "DAEMON_FAIL_START_NONEXIST",
	.description = "Could not launch the daemon, the executable was not "
	    "found.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_FAIL_STOPPING = {
	.name = "DAEMON_FAIL_STOPPING",
	.description = "This daemon failed to stop.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_UP_CHECK_FAILED = {
	.name = "DAEMON_UP_CHECK_FAILED",
	.description = "The last checks done before starting the daemon "
	    "failed.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h DAEMON_RESPAWN_RATE_EXCEEDED = {
	.name = "DAEMON_RESPAWN_RATE_EXCEEDED",
	.description = "This daemon has been respawned too many times, and "
	    "will be disabled.",
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

/* This are run, when initng wants to start a daemon */
static int start_DAEMON(active_db_h * daemon_to_start)
{
	D_("Starting daemon %s.\n", daemon_to_start->name);

	/* if its waiting for deps to stop, we can reset it running again */
	if (IS_MARK(daemon_to_start, &DAEMON_WAITING_FOR_STOP_DEP)) {
		initng_common_mark_service(daemon_to_start, &DAEMON_RUNNING);
		return TRUE;
	}

	/* mark it WAITING_FOR_START_DEP and wait */
	if (!initng_common_mark_service(daemon_to_start, &DAEMON_START_MARKED)) {
		W_("mark_daemon DAEMON_START_MARKED failed for daemon %s\n",
		   daemon_to_start->name);
		return FALSE;
	}

	/* return happily */
	return TRUE;
}

/* This are run, when initng wants to stop a daemon */
static int stop_DAEMON(active_db_h * daemon)
{
	/* if its waiting for deps to start, we can set it to stopped directly */
	if (IS_MARK(daemon, &DAEMON_WAITING_FOR_START_DEP)) {
		initng_common_mark_service(daemon, &DAEMON_STOPPED);
		return TRUE;
	}

	/* set stopping */
	if (!initng_common_mark_service(daemon, &DAEMON_STOP_MARKED)) {
		W_("mark_service DAEMON_STOP_MARKED failed for daemon %s.\n",
		   daemon->name);
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
	/* Add a new servicetype */
	initng_service_type_register(&TYPE_DAEMON);

	/* Add 2 new processtype */
	initng_process_db_ptype_register(&T_DAEMON);
	initng_process_db_ptype_register(&T_KILL);

	/* Add some new variables */
	initng_service_data_type_register(&PIDFILE);
	initng_service_data_type_register(&PIDOF);
	initng_service_data_type_register(&FORKS);
	initng_service_data_type_register(&RESPAWN);
	initng_service_data_type_register(&TERM_TIMEOUT);
	initng_service_data_type_register(&DAEMON_FAIL_OK);
	initng_service_data_type_register(&DAEMON_STOPS_BADLY);
	initng_service_data_type_register(&INTERNAL_LAST_RESPAWN);
	initng_service_data_type_register(&RESPAWN_PAUSE);
	initng_service_data_type_register(&RESPAWN_RATE);

	/* Add some new service-states */
	initng_active_state_register(&DAEMON_START_MARKED);
	initng_active_state_register(&DAEMON_STOP_MARKED);
	initng_active_state_register(&DAEMON_RUNNING);
	initng_active_state_register(&DAEMON_WAITING_FOR_START_DEP);
	initng_active_state_register(&DAEMON_WAITING_FOR_STOP_DEP);
	initng_active_state_register(&DAEMON_START_DEPS_MET);
	initng_active_state_register(&DAEMON_STOP_DEPS_MET);
	initng_active_state_register(&DAEMON_KILL);
	initng_active_state_register(&DAEMON_TERM);
	initng_active_state_register(&DAEMON_STOPPED);
	initng_active_state_register(&DAEMON_LAUNCH);
	initng_active_state_register(&DAEMON_WAIT_FOR_PID_FILE);
	initng_active_state_register(&DAEMON_START_DEPS_FAILED);
	initng_active_state_register(&DAEMON_STOP_DEPS_FAILED);
	initng_active_state_register(&DAEMON_FAIL_START_RCODE);
	initng_active_state_register(&DAEMON_FAIL_START_SIGNAL);
	initng_active_state_register(&DAEMON_FAIL_START_LAUNCH);
	initng_active_state_register(&DAEMON_FAIL_START_NONEXIST);
	initng_active_state_register(&DAEMON_FAIL_START_TIMEOUT_PIDFILE);
	initng_active_state_register(&DAEMON_WAITING_FOR_UP_CHECK);

	initng_active_state_register(&DAEMON_FAIL_STOPPING);
	initng_active_state_register(&DAEMON_WAIT_RESP_TOUT);
	initng_active_state_register(&DAEMON_UP_CHECK_FAILED);
	initng_active_state_register(&DAEMON_RESPAWN_RATE_EXCEEDED);

	/* return happily */
	return TRUE;
}

void module_unload(void)
{
	/* Remove all added states */
	initng_active_state_unregister(&DAEMON_START_MARKED);
	initng_active_state_unregister(&DAEMON_STOP_MARKED);
	initng_active_state_unregister(&DAEMON_RUNNING);
	initng_active_state_unregister(&DAEMON_WAITING_FOR_START_DEP);
	initng_active_state_unregister(&DAEMON_WAITING_FOR_STOP_DEP);
	initng_active_state_unregister(&DAEMON_START_DEPS_MET);
	initng_active_state_unregister(&DAEMON_STOP_DEPS_MET);
	initng_active_state_unregister(&DAEMON_KILL);
	initng_active_state_unregister(&DAEMON_TERM);
	initng_active_state_unregister(&DAEMON_STOPPED);
	initng_active_state_unregister(&DAEMON_LAUNCH);
	initng_active_state_unregister(&DAEMON_WAIT_FOR_PID_FILE);
	initng_active_state_unregister(&DAEMON_START_DEPS_FAILED);
	initng_active_state_unregister(&DAEMON_STOP_DEPS_FAILED);
	initng_active_state_unregister(&DAEMON_FAIL_START_RCODE);
	initng_active_state_unregister(&DAEMON_FAIL_START_SIGNAL);
	initng_active_state_unregister(&DAEMON_FAIL_START_LAUNCH);
	initng_active_state_unregister(&DAEMON_FAIL_START_NONEXIST);
	initng_active_state_unregister(&DAEMON_FAIL_START_TIMEOUT_PIDFILE);
	initng_active_state_unregister(&DAEMON_FAIL_STOPPING);
	initng_active_state_unregister(&DAEMON_WAIT_RESP_TOUT);
	initng_active_state_unregister(&DAEMON_UP_CHECK_FAILED);
	initng_active_state_unregister(&DAEMON_RESPAWN_RATE_EXCEEDED);
	initng_active_state_unregister(&DAEMON_WAITING_FOR_UP_CHECK);

	/* Delete all added variables */
	initng_service_data_type_unregister(&PIDFILE);
	initng_service_data_type_unregister(&PIDOF);
	initng_service_data_type_unregister(&FORKS);
	initng_service_data_type_unregister(&RESPAWN);
	initng_service_data_type_unregister(&TERM_TIMEOUT);
	initng_service_data_type_unregister(&DAEMON_STOPS_BADLY);
	initng_service_data_type_unregister(&DAEMON_FAIL_OK);
	initng_service_data_type_unregister(&INTERNAL_LAST_RESPAWN);
	initng_service_data_type_unregister(&RESPAWN_PAUSE);
	initng_service_data_type_unregister(&RESPAWN_RATE);

	/* Delete the processstypes */
	initng_process_db_ptype_unregister(&T_DAEMON);
	initng_process_db_ptype_unregister(&T_KILL);

	/* Last, delete the servicetype */
	initng_service_type_unregister(&TYPE_DAEMON);
}

/*
 * ############################################################################
 * #                         STATE_FUNCTIONS                                  #
 * ############################################################################
 */

/*
 * Everything DAEMON_START_MARKED are gonna do, is to set it DAEMON_WAITING_FOR_START_DEP
 */
static void init_DAEMON_START_MARKED(active_db_h * daemon)
{

	/* start all our dependencies */
	initng_depend_start_deps(daemon);

	/* put it in DAEMON_WAITING_FOR_START_DEP */
	initng_common_mark_service(daemon, &DAEMON_WAITING_FOR_START_DEP);
}

/*
 * Everything DAEMON_STOP_MARKED are gonna do, is to set it DAEMON_WAITING_FOR_STOP_DEP
 */
static void init_DAEMON_STOP_MARKED(active_db_h * daemon)
{
	/* stop all our dependencies */
	initng_depend_stop_deps(daemon);

	/* put it in DAEMON_WAITING_FOR_STOP_DEP */
	initng_common_mark_service(daemon, &DAEMON_WAITING_FOR_STOP_DEP);
}

static void handle_DAEMON_WAITING_FOR_START_DEP(active_db_h * daemon)
{
	assert(daemon);

	/*
	 * this checks depencncy.
	 * initng_depend_start_dep_met() will return:
	 * TRUE (dep is met)
	 * FALSE (dep is not met)
	 * FAIL (dep is failed)
	 */
	switch (initng_depend_start_dep_met(daemon, FALSE)) {
	case TRUE:
		break;

	case FAIL:
		initng_common_mark_service(daemon, &DAEMON_START_DEPS_FAILED);
		return;

	default:
		/* return and hope that this handler will be called again. */
		return;
	}

	/* if system is shutting down, Don't start anything. */
	if (g.sys_state != STATE_STARTING && g.sys_state != STATE_UP) {
		F_("Can't start daemon, when system status is: %i !\n",
		   g.sys_state);
		initng_common_mark_service(daemon, &DAEMON_STOPPED);
		return;
	}

	/* set status to START_DEP_MET */
	initng_common_mark_service(daemon, &DAEMON_START_DEPS_MET);
}

static void handle_DAEMON_WAITING_FOR_STOP_DEP(active_db_h * daemon)
{
	assert(daemon);

	/* check with other plug-ins, if it is ok to stop this service now */
	if (initng_depend_stop_dep_met(daemon, FALSE) != TRUE)
		return;

	/* ok, stopping deps are met */
	initng_common_mark_service(daemon, &DAEMON_STOP_DEPS_MET);
}

static void init_DAEMON_START_DEPS_MET(active_db_h * daemon)
{
	/* clear all stale pidfiles if any */
	clear_pidfile(daemon);

	/* set the DAEMON_LAUNCH state */
	if (!initng_common_mark_service(daemon, &DAEMON_LAUNCH))
		return;

	/*
	 * If pidof is set, check that there is no
	 * existing running daemon in here.
	 */
	if (is(&PIDOF, daemon)) {
		pid_t pid = 0;

		D_("getting pid by PIDOF!\n");
		/* get pid by process name */
		pid = get_pidof(daemon);
		D_("result : %d\n", pid);

		/* if the pid really exist on the system */
		if (pid > 1 && kill(pid, 0) == 0) {
			/* create an new process */
			process_h *existing_process =
			    initng_process_db_new(&T_DAEMON);

			if (existing_process) {
				W_("Daemon for service %s was already "
				   "running, adding it to service daemon "
				   "process list instead of starting a new "
				   "one.\n", daemon->name);

				/* set process status */
				existing_process->pid = pid;

				/* add process */
				initng_process_db_register_to_service
				    (existing_process, daemon);
				initng_common_mark_service(daemon,
							   &DAEMON_RUNNING);
				return;
			}
		}
	}

	/* F I N A L L Y   S T A R T */
	switch (initng_execute_launch(daemon, &T_DAEMON, NULL)) {
	case FALSE:
		F_("Did not find a service->\"daemon\" entry for %s to run!\n",
		   daemon->name);
		initng_common_mark_service(daemon, &DAEMON_FAIL_START_LAUNCH);
		return;

	case FAIL:
		F_("Service %s, could not launch service->\"daemon\"\n",
		   daemon->name);
		initng_common_mark_service(daemon, &DAEMON_FAIL_START_NONEXIST);
		return;

	default:
		break;
	}

	/* if PIDFILE or PIDOF set, put daemon in WAIT_FOR_PIDFILE_STATE */
	if (is(&PIDFILE, daemon) || is(&PIDOF, daemon)) {
		initng_common_mark_service(daemon, &DAEMON_WAIT_FOR_PID_FILE);
		return;
	};

	/* If daemon is a forking one, let it stay DAEMON_LAUNCH */
	if (is(&FORKS, daemon)) {
		D_("FORKS is set, will wait for return.\n");
		return;
	}

	D_("FORKS not set, setting to DAEMON_RUNNING directly.\n");

	initng_common_mark_service(daemon, &DAEMON_WAITING_FOR_UP_CHECK);
}

static void init_DAEMON_STOP_DEPS_MET(active_db_h * service)
{
	process_h *process = NULL;

	/* find the daemon, and check so it still exits */
	if (!(process = initng_process_db_get(&T_DAEMON, service))) {
		F_("Could not find process to kill!\n");
		initng_common_mark_service(service, &DAEMON_STOPPED);
		return;
	}

	/* Check so process have a valid pid */
	if (process->pid <= 0) {
		D_("Pid is unvalid, marked as DAEMON_STOPPED\n");
		initng_common_mark_service(service, &DAEMON_STOPPED);
		return;
	}

	if (kill(process->pid, 0) && errno == ESRCH) {
		D_("Dont exist a process with pid %i, mark as "
		   "DAEMON_STOPPED\n", process->pid);
		initng_common_mark_service(service, &DAEMON_STOPPED);
		return;
	}

	/* launch stop service */
	switch (initng_execute_launch(service, &T_KILL, NULL)) {
	case FAIL:
		F_("  --  (%s): fail launch stop!\n", service->name);
		initng_common_mark_service(service, &DAEMON_FAIL_STOPPING);
		return;

	case FALSE:
		{
			/* if there is no module that wanna kill this
			 * daemon, we do it ourself. */
			kill_daemon(service, SIGTERM);
			if (!initng_common_mark_service(service, &DAEMON_TERM))
				return;
			break;
		}

	default:
		/* Set its state to GOING_KILLED, to mark that we put
		 * some effort on killing it. */
		if (!initng_common_mark_service(service, &DAEMON_TERM))
			return;
		break;
	}
}

/*
 * Set an alarm to 1 seconds .
 */
static void init_DAEMON_WAIT_FOR_PID_FILE(active_db_h * s)
{
	/* Set the alarm to 1 seconds, to make a pidfile check every sec */
	initng_handler_set_alarm(s, 1);
}

/*
 * Timeout reached.
 *
 * What to do when timeout reached.
 */
static void timeout_DAEMON_WAIT_FOR_PID_FILE(active_db_h * s)
{
	/* check if timeout have appeared */
	if (g.now.tv_sec > s->time_current_state.tv_sec + PID_TIMEOUT) {
		process_h *process = NULL;

		F_("Service \"%s\" wait for pidfile timed out! Will kill "
		   "daemon now.\n", s->name);

		initng_common_mark_service(s,
					   &DAEMON_FAIL_START_TIMEOUT_PIDFILE);
		kill_daemon(s, SIGKILL);

		/* Free the process if found */
		process = initng_process_db_get(&T_DAEMON, s);
		if (process) {
			initng_process_db_free(process);
		}

		return;
	}

	/*
	 * NOW, start check for a pid
	 */
	if (!try_get_pid(s)) {
		/* try again in 1 second */
		initng_handler_set_alarm(s, 1);
	}
}

/*
 * When a service has quit, and should respwan, it is set to this state.
 * in the wait for timeout to respawn.
 */
static void init_DAEMON_WAIT_RESP_TOUT(active_db_h * service)
{
	int respawn_pause = DEFAULT_RESPAWN_PAUSE;

	/* if RESPAWN_PAUSE set */
	if (is(&RESPAWN_PAUSE, service)) {
		/* get it */
		respawn_pause = get_int(&RESPAWN_PAUSE, service);
	}

	D_("Will sleep %i seconds before respawning!\n", respawn_pause);
	initng_handler_set_alarm(service, respawn_pause);
}

/* When respawn timeout is out, reset service and start it */
static void timeout_DAEMON_WAIT_RESP_TOUT(active_db_h * service)
{
	/* If set it stopped, so it can be restarted */
	initng_common_mark_service(service, &DAEMON_STOPPED);

	/* to make sure it will be started */
	initng_handler_start_service(service);
}

/* set timeout */
static void init_DAEMON_TERM(active_db_h * daemon)
{
	int timeout;

	/* get the TERM_TIMEOUT */
	timeout = get_int(&TERM_TIMEOUT, daemon);

	/* Make sure we got an value */
	if (timeout == 0)
		timeout = DEFAULT_TERM_TIMEOUT;

	initng_handler_set_alarm(daemon, timeout);
}

/* when DAEMON_TERM timeout, kill it instead */
static void timeout_DAEMON_TERM(active_db_h * daemon)
{
	F_("Service %s TERM_TIMEOUT reached!, sending KILL signal.\n",
	   daemon->name);
	kill_daemon(daemon, SIGKILL);

	/* Set it to DAEMON_KILL state, monitoring that TERM signal is
	 * sent. */
	initng_common_mark_service(daemon, &DAEMON_KILL);
}

/* set timeout when a service sets to state DAEMON_KILL */
static void init_DAEMON_KILL(active_db_h * daemon)
{
	initng_handler_set_alarm(daemon, DEFAULT_KILL_TIMEOUT);
}

/* set SIGKILL on every timeout */
static void timeout_DAEMON_KILL(active_db_h * daemon)
{
	F_("Service %s KILL_TIMEOUT of %i seconds reached! "
	   "(%i seconds passed), sending another KILL signal.\n",
	   daemon->name, DEFAULT_KILL_TIMEOUT,
	   MS_DIFF(g.now, daemon->time_current_state));
	kill_daemon(daemon, SIGKILL);

	/* Dont be afraid to kill again */
	initng_handler_set_alarm(daemon, DEFAULT_KILL_TIMEOUT);
}

static void check_DAEMON_UP(active_db_h * daemon)
{
	switch (initng_depend_up_check(daemon)) {
	case FALSE:
		/* NOT UP YET! return and hope that this handler will be called*/
		break;

	case FAIL:
		/* FAIL! */
		initng_common_mark_service(daemon, &DAEMON_UP_CHECK_FAILED);
		break;

	default:
		/* OK! now daemon is RUNNING! */
		initng_common_mark_service(daemon, &DAEMON_RUNNING);
		break;
	}
}

/*
 * ############################################################################
 * #                         KILL HANDLER FUNCTIONS                            #
 * ############################################################################
 */

static void handle_killed_daemon(active_db_h * daemon, process_h * process)
{
	assert(daemon);
	assert(daemon->name);
	assert(daemon->current_state);
	assert(daemon->current_state->name);
	assert(process);
	int rcode;

	D_("handle_killed_start(%s): initial status: \"%s\".\n",
	   daemon->name, daemon->current_state->name);

	/*
	 * Youst run if we checks for PID FILE
	 */
	if (daemon->current_state == &DAEMON_WAIT_FOR_PID_FILE &&
	    is(&FORKS, daemon)) {
		try_get_pid(daemon);
		return;
	}

	/* This service are gonna respawn, no need to set it stopped */
	if (check_respawn(daemon)) {
		/* It is gonna respawn, free this process and forget */
		initng_process_db_free(process);
		return;
	}

	/* Set local rcode, and free process */
	rcode = process->r_code;
	initng_process_db_free(process);

	/*
	 * Make sure r_code don't signal error (can be override by UP_ON_FAILURE.
	 */

	/* If daemon stoped by a SEGFAULT (SIGSEGV==11) this is always bad */
	if (WTERMSIG(rcode) == 11) {
		initng_common_mark_service(daemon, &DAEMON_FAIL_START_SIGNAL);
		return;
	}

	/* if exit with sucess */
	if (WEXITSTATUS(rcode) == 0) {
		/* OK! now daemon is STOPPED! */
		initng_common_mark_service(daemon, &DAEMON_STOPPED);
		return;
	}

	/* else exitstatus is abouw 0 */
	if (is(&DAEMON_FAIL_OK, daemon)) {
		/* It might be ok anyway */
		initng_common_mark_service(daemon, &DAEMON_STOPPED);
		return;
	}

	/* This is also ok */
	if (daemon->current_state->is == IS_STOPPING &&
	    is(&DAEMON_STOPS_BADLY, daemon)) {
		/* It might be ok anyway */
		initng_common_mark_service(daemon, &DAEMON_STOPPED);
		return;
	}

	/* else fail by RCODE failure */
	initng_common_mark_service(daemon, &DAEMON_FAIL_START_RCODE);
}

/*
 * ############################################################################
 * #                         LOCAL FUNCTIONS                                  #
 * ############################################################################
 */

static void kill_daemon(active_db_h * service, int sig)
{
	process_h *process = NULL;

	assert(service);

	/* make sure we got a process */
	if (!(process = initng_process_db_get(&T_DAEMON, service))) {
		F_("Service doesn't have any processes, don't know how to "
		   "kill then.\n");
		return;
	}

	/* check so that pid is good */
	if (process->pid <= 0) {
		F_("Bad PID %d in database!\n", process->pid);
		initng_process_db_free(process);
		return;
	}

	/* check so there exits an process with this pid */
	if (kill(process->pid, 0) && errno == ESRCH) {
		F_("Trying to kill a service (%s) with a pid (%d), but there "
		   "exists no process with this pid!\n",
		   service->name, process->pid);
		initng_process_db_free(process);
		return;
	}

	/* if system is not stopping, generate a warning */
	/*if (g.sys_state != STATE_STOPPING)
	   {
	   W_(" Sending the process %i of %s, the SIGTERM signal!\n",
	   process->pid, service->name);
	   } */

	/* Uhm, this doesn't work : kill(-service->start_process->pid, SIGKILL); */
	kill(process->pid, sig);
}

/*
 * pid_of(name)
 *  This function will walk /proc all numbers dirs, looking in the stat file
 *  for the process name, if it matches name, it will return the pid of it.
 *  If it not succeeds to find it, it will return(-1).
 */
static pid_t pid_of(const char *name)
{
	DIR *dir;
	struct dirent *d;
	FILE *fp;

	/* maximum possible length for string "/proc/12232/stat" can be */
#define BUFF_SIZE 512

	D_("Will fetch pid of \"%s\"\n", name);

	/* Open /proc or fail */
	if (!(dir = opendir("/proc")))
		return (-1);

	/* Walk through the directory. */
	while ((d = readdir(dir))) {
		char buf[BUFF_SIZE + 1];	/* Will contain a fixed string
						 * like "/proc/12232/stat" */
		char *s = NULL;	/* Temporary pointer when
				 * parsing the content of the
				 * stat file */
		int len = 0;	/* An length counter */
		pid_t pid = -1;	/* Will contain the pid to
				 * return */

		/* Make sure this dirname is a number == pid */
		pid = atoi(d->d_name);
		if (pid <= 0)
			continue;

		/* Fix a string, that matches the full path of the stat
		 * file */
		snprintf(buf, BUFF_SIZE, "/proc/%d/stat", pid);
		D_("To open: %s\n", buf);

		/* Read SID & statname from it or fail */
		if (!(fp = fopen(buf, "r"))) {
			W_("Could not open %s.\n", buf);
			continue;
		}

		/* fetch the full stat file, or fail */
		if (!fgets(buf, BUFF_SIZE, fp)) {
			fclose(fp);
			continue;
		}

		/* close stat file */
		fclose(fp);

		/* set the walk counter, to the start of the file content fetched */
		s = buf;

		/* skip all chars to the first space - first string contains the pid no */
		while (*s && *s != ' ')
			s++;

		/* Make sure we have any data */
		if (*s == '\0')
			continue;

		/* skip the space */
		s++;

		/* skip the '(' char */
		if (*s != '(')
			continue;
		s++;

		/* count the length */
		while (s[len] && s[len] != ')')
			len++;

		/* compare the name in the '(' ')' chars with the process
		 * name we are looking for */
		if (strncmp(s, name, len) == 0) {
			D_("Found %s with pid %d\n", name, pid);

			/* make sure the dir (/proc) is closed. */
			if (dir)
				closedir(dir);

			/* return happily with the pid */
			return (pid);
		}
	}

	/* close the dir (/proc) if still open */
	if (dir)
		closedir(dir);

	D_("Did not find a process with name \"%s\"\n", name);
	return (-1);
}

/*
 * Check if a pidfile exists, if it exists, update the
 * pid in the active_db entry. and return TRUE
 */
static pid_t pid_from_file(const char *name)
{
	unsigned int i = 0;
	pid_t ret = 0;
	int fd = 0;
	unsigned int len = 0;
	char buf[21];

	assert(name);

	/* open pid file */
	fd = open(name, O_RDONLY);

	/* If we cant open pidfile, this is bad */
	if (fd == -1)
		return -1;

	/* Read data from buffer */
	len = read(fd, buf, 20);
	close(fd);
	if (len < 1) {
		F_("Read 0 chars from %s, It's empty.\n", name);
		return -1;
	}

	for (i = 0; i < len && !isdigit(buf[i]); i++) ;

	for (; i < len && isdigit(buf[i]); i++)
		ret = ret * 10 - '0' + buf[i];

	if (!ret && i > len) {
		F_("bufferoverrun: no pid fetched from '%s'.", name);
	} else if (ret && i > len) {
		F_("bufferoverrun: the pid received from '%s' is pid (%i).",
		   name, ret);
	}

	return ret;
}

static pid_t get_pidof(active_db_h * s)
{
	const char *pidof;

	pidof = get_string(&PIDOF, s);
	if (!pidof)
		return -1;

	return pid_of(pidof);
}

/* this will get the pid of PIDFILE entry of service */
static pid_t get_pidfile(active_db_h * s)
{
	pid_t pid;
	const char *pidfile = NULL;
	s_data *itt = NULL;

	/* get the pidfile */
	while ((pidfile = get_next_string(&PIDFILE, s, &itt))) {
		/* make sure the first char is a '/' so its a full path */
		if (pidfile[0] != '/') {
			F_("%s has pid_file with relative path \"%s\"\n",
			   s->name, pidfile);
			/* check_valid_pidfile_path() can detect certain
			 * dangerous typos, but it can't prevent loading.
			 * Stop to be safe */
			return -1;
		}

		/* get the pid from the file */
		pid = pid_from_file(pidfile);

		/* return the pid */
		if (pid > 1)
			return (pid);
	}

	return -1;
}

static void clear_pidfile(active_db_h * s)
{
	const char *pidfile = NULL;
	s_data *itt = NULL;

	/* ok, search for pidfiles */
	while ((pidfile = get_next_string(&PIDFILE, s, &itt))) {
		if (pidfile) {
			/* check it's an absolute path after variable substitution */
			if (pidfile[0] == '/') {
				if (unlink(pidfile) != 0 && errno != ENOENT) {
					F_("Could not remove stale pidfile "
					   "\"%s\"\n", pidfile);
				}
				break;
			} else {
				F_("%s has pid_file with relative path "
				   "\"%s\"\n", s->name, pidfile);
				/* check_valid_pidfile_path() can detect
				 * certain dangerous typos, but it can't
				 * prevent loading. Stop to be safe */
				return;
			}
		}
	}
}

/*
 * ############################################################################
 * #                         RESPAWN FUNCTIONS                                #
 * ############################################################################
 */

/*
 * This is a check if service will be set in a respawn mode.
 * Will return TRUE if respawn is handled.
 * Else false and function can set it into stopped mode.
 */
static int check_respawn(active_db_h * service)
{
	time_t last = 0;
	int respawn_rate = DEFAULT_RESPAWN_RATE;

	assert(service);

	/* Check so service status is DAEMON_RUNNING, or it wont have
	 * stopped by it self */
	if (!IS_MARK(service, &DAEMON_RUNNING))
		return FALSE;

	/* check if the service have respawn enabled */
	if (!is(&RESPAWN, service)) {
		D_("Service %s doesn't have RESPAWN flag set, won't "
		   "respawn!\n", service->name);
		return FALSE;
	}

	/* get times */
	if (is(&INTERNAL_LAST_RESPAWN, service))
		last = (time_t) get_int(&INTERNAL_LAST_RESPAWN, service);
	D_("Now: %i , Last: %i\n", (int)g.now.tv_sec, (int)last);

	/* get respawn_rate if set */
	if (is(&RESPAWN_RATE, service)) {
		respawn_rate = get_int(&RESPAWN_RATE, service);
	}

	/* make sure it wont respawn to often */
	if (last && (respawn_rate > 0)) {
		/* if times pased is less then respawn_rate */
		if ((g.now.tv_sec - last) < respawn_rate) {
			W_("Won't respawn service %s, it was respawned %i "
			   "seconds ago.\n", service->name,
			   (int)(g.now.tv_sec - last));

			initng_common_mark_service(service,
						   &DAEMON_RESPAWN_RATE_EXCEEDED);

			return FALSE;
		}
	}

	/* set the next INTERNAL_LAST_RESPAWN no to use. */
	set_int(&INTERNAL_LAST_RESPAWN, service, (int)g.now.tv_sec);

	initng_common_mark_service(service, &DAEMON_WAIT_RESP_TOUT);
	return TRUE;
}

/*
 * This function tries to update the process pid from a pidfile or pidof
 */
static int try_get_pid(active_db_h * s)
{
	pid_t pid = -1;

	D_("Trying to get pid of %s\n", s->name);

	/* Try get the pid from PIDOF is set */
	if (is(&PIDOF, s)) {
		D_("getting pid by PIDOF!\n");
		/* get pid by process name */
		pid = get_pidof(s);
		D_("result : %d\n", pid);
		/* Try get the pid from PIDFILE if set */
	} else if (is(&PIDFILE, s)) {
		D_("getting pid by PIDFILE!\n");
		pid = get_pidfile(s);
		D_("result : %d\n", pid);
	} else {
		F_("No one of PIDOF or PIDFILE are set, but initng is "
		   "waiting for a pid.\n");
		return FALSE;
	}

	/* check if a process with that pid exits in the system */
	if (pid > 0 && kill(pid, 0) < 0 && (errno == ESRCH)) {
		F_("Got a non-existent pid %i for daemon \"%s\", maybe there "
		   "is a stale pidfile around.", pid, s->name);
		pid = -1;	/* reset */
	} else if (pid > 0) {
		process_h *p = NULL;

		/* get the process to handle */
		if (!(p = initng_process_db_get(&T_DAEMON, s))) {
			F_("Did not find a daemon process on this "
			   "service!\n");
			initng_common_mark_service(s, &DAEMON_UP_CHECK_FAILED);
			return FALSE;
		}

		/* finally set the new pid - but not if forks=no, because
		   that can cause problems */
		if (is(&FORKS, s))
			p->pid = pid;

		initng_common_mark_service(s, &DAEMON_WAITING_FOR_UP_CHECK);

		/* return HAPPILY */
		return TRUE;
	}

	return FALSE;
}
