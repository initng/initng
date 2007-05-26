/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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
#include <stdlib.h>							/* free() exit() */
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>


INITNG_PLUGIN_MACRO;


/*
 * ############################################################################
 * #                        STATE HANDLERS FUNCTION DEFINES                   #
 * ############################################################################
 */

static void task_deps_met(active_db_h *task);
static void handle_deps(active_db_h *task);
static void task_run_marked(active_db_h *task);
static int start_task(active_db_h *task);

/*
 * ############################################################################
 * #                        PROCESS TYPE FUNCTION DEFINES                     #
 * ############################################################################
 */
static void handle_task_leave(active_db_h * killed_task, process_h * process);


/*
 * ############################################################################
 * #                     Official SERVICE_TYPE STRUCT                         #
 * ############################################################################
 */
stype_h TYPE_TASK = {
	"task", "This is a task, when active it will lissen on trigger "
	        "requirement.",
	FALSE, &start_task, NULL, NULL
};

/*
 * ############################################################################
 * #                       PROCESS TYPES STRUCTS                              #
 * ############################################################################
 */
ptype_h RUN_TASK = { "task", &handle_task_leave };

/*
 * ############################################################################
 * #                        ACTIVE VARIABLES                                  #
 * ############################################################################
 */

s_entry ONCE = { "once", SET, &TYPE_TASK, "Task should run only once" };

/*
 * ############################################################################
 * #                          ACTIVE STATES STRUCTS                           #
 * ############################################################################
 */

/*
 * When we have a active task waiting for its requirements.
 */
a_state_h TASK_WAITING = {
	"TASK_WAITING", "This task is waiting for a trigger.",
	IS_DOWN, NULL, NULL, NULL
};

/*
 * When a task called, it will get this state.
 */
a_state_h TASK_RUNNING = {
	"TASK_RUNNING", "This task is running.",
	IS_STARTING, NULL, NULL, NULL
};

/*
 * When a task has run.
 */
a_state_h TASK_DONE = {
	"TASK_DONE", "Task completed.",
	IS_UP, NULL, NULL, NULL
};

/*
 * When a task has failed.
 */
a_state_h TASK_FAILED = {
	"TASK_FAILED", "When task has failed.",
	IS_FAILED, NULL, NULL, NULL
};

a_state_h TASK_RUN_MARKED = {
	"TASK_RUN_MARKED", "When task is marked for running.",
	IS_STARTING, NULL, &task_run_marked, NULL
};

a_state_h TASK_DEPS_FAILED = {
	"TASK_DEPS_FAILED", "One of the depdencencies this task requires "
	                    "has failed, this task can't start.",
	IS_FAILED, NULL, NULL, NULL
};

a_state_h TASK_WAITING_FOR_DEPS = {
	"TASK_WAITING_FOR_DEPS", "This service is waiting for all its "
	                         "depdencencies to be met.",
	IS_STARTING, &handle_deps, NULL, NULL
};

a_state_h TASK_DEPS_MET = {
	"TASK_DEPS_MET", "The dependencies for starting this service are met.",
	IS_STARTING, NULL, &task_deps_met, NULL
};

/*
 * ############################################################################
 * #                     ACTIVE FAILING STATES STRUCTS                        #
 * ############################################################################
 */


/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTIONS                         #
 * ############################################################################
 */


/*
 * ############################################################################
 * #                         PLUGIN INITIATORS                               #
 * ############################################################################
 */


int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION) {
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return FALSE;
	}

	initng_service_type_register(&TYPE_TASK);

	initng_process_db_ptype_register(&RUN_TASK);

	initng_active_state_register(&TASK_WAITING);
	initng_active_state_register(&TASK_RUNNING);
	initng_active_state_register(&TASK_FAILED);
	initng_active_state_register(&TASK_DONE);
	initng_active_state_register(&TASK_RUN_MARKED);
	initng_active_state_register(&TASK_DEPS_FAILED);
	initng_active_state_register(&TASK_WAITING_FOR_DEPS);
	initng_active_state_register(&TASK_DEPS_MET);

	initng_service_data_type_register(&ONCE);

	return TRUE;
}

void module_unload(void)
{
	D_("module_unload();\n");

	initng_service_type_unregister(&TYPE_TASK);

	initng_process_db_ptype_unregister(&RUN_TASK);

	initng_active_state_unregister(&TASK_WAITING);
	initng_active_state_unregister(&TASK_RUNNING);
	initng_active_state_unregister(&TASK_FAILED);
	initng_active_state_unregister(&TASK_DONE);
	initng_active_state_unregister(&TASK_RUN_MARKED);
	initng_active_state_unregister(&TASK_DEPS_FAILED);
	initng_active_state_unregister(&TASK_WAITING_FOR_DEPS);
	initng_active_state_unregister(&TASK_DEPS_MET);

	initng_service_data_type_unregister(&ONCE);
}

/*
 * ############################################################################
 * #                         STATE_FUNCTIONS                                  #
 * ############################################################################
 */

/* to do when task is done */
static void handle_task_leave(active_db_h * killed_task, process_h * process)
{
	if (is(&ONCE, killed_task)) {
		initng_common_mark_service(killed_task, &TASK_DONE);
	} else {
		initng_process_db_free(process);
		initng_common_mark_service(killed_task, &TASK_WAITING);
	}
}

static int start_task(active_db_h *task)
{
	/* Only mark it if it's waiting */
	if (IS_MARK(task, &TASK_WAITING))
		initng_common_mark_service(task, &TASK_RUN_MARKED);

	return TRUE;
}

static void task_run_marked(active_db_h *task)
{
	/* Start our dependencies */
	if (initng_depend_start_deps(task) != TRUE) {
		initng_common_mark_service(task, &TASK_DEPS_FAILED);
		return;
	}

	/* set WAITING_FOR_START_DEP */
	initng_common_mark_service(task, &TASK_WAITING_FOR_DEPS);
}


static void handle_deps(active_db_h *task)
{
	switch (initng_depend_start_dep_met(task, FALSE))
	{
		case TRUE:
			break;
		case FAIL:
			initng_common_mark_service(task, &TASK_DEPS_FAILED);
		default:
			return;
	}

	initng_common_mark_service(task, &TASK_DEPS_MET);
}

static void task_deps_met(active_db_h *task)
{
	initng_common_mark_service(task, &TASK_RUNNING);

	switch (initng_execute_launch(task, &RUN_TASK, NULL)) {
		case FALSE:
			F_("Did not find a task entry to run\n");
			initng_common_mark_service(task, &TASK_FAILED);
			break;
		case FAIL:
			F_("Could not launch task\n");
			initng_common_mark_service(task, &TASK_FAILED);
			break;
	}
}
