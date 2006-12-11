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
#include <assert.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
/*#include <time.h> */

#include <initng_env_variable.h>
#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <initng_common.h>
#include <initng_toolbox.h>
#include <initng_static_data_id.h>
#include <initng_static_states.h>
#include <initng_static_service_types.h>
#include <initng_depend.h>
#include <initng_execute.h>
#include <initng_event_hook.h>
#include <initng_static_event_types.h>
#include <initng_active_db.h>


INITNG_PLUGIN_MACRO;


static void handle_event(s_event * extrn_event);
static int event_triggerer(active_db_h * service);

/*
 * ############################################################################
 * #                               EVENT STUFF                                #
 * ############################################################################
 */

s_event_type EXTRN_EVENT = { "EXTRN_EVENT", "External event, triggered and handled by ifiles" };

typedef struct {
	s_event_type * event_type;
	char * triggerer;
	char * target;
} s_extrn_event;


/*
 * ############################################################################
 * #                        STATE HANDLERS FUNCTION DEFINES                   #
 * ############################################################################
 */

/*
 * ############################################################################
 * #                        PROCESS TYPE FUNCTION DEFINES                     #
 * ############################################################################
 */
static void handle_event_leave(active_db_h * killed_event, process_h * process);


/*
 * ############################################################################
 * #                     Official SERVICE_TYPE STRUCT                         #
 * ############################################################################
 */
stype_h TYPE_EVENT = { "event", "This is an event, when active it will lissen on trigger requirement.", FALSE, NULL, NULL, NULL };

/*
 * ############################################################################
 * #                       PROCESS TYPES STRUCTS                              #
 * ############################################################################
 */
ptype_h RUN_EVENT = { "run_event", &handle_event_leave };

/*
 * ############################################################################
 * #                        ACTIVE VARIABLES                                  #
 * ############################################################################
 */

s_entry TRIGGER = { "trigger", STRINGS, &TYPE_EVENT, "Add requirements here for events to run" };
s_entry AUTO_RESET = { "reset_trigger", INT, &TYPE_EVENT, "Reset event when done" };

/*
 * ############################################################################
 * #                          ACTIVE STATES STRUCTS                           #
 * ############################################################################
 */

/*
 * When we have a active event waiting for its requirements.
 */
a_state_h EVENT_WAITING = { "EVENT_WAITING", "This event is waiting for a trigger.", IS_DOWN, NULL, NULL, NULL };

/*
 * When a event is triggered, it will get this state.
 */
a_state_h EVENT_RUNNING = { "EVENT_RUNNING", "This service is marked for stopping.", IS_STARTING, NULL, NULL, NULL };

/*
 * When a event has run.
 */
a_state_h EVENT_DONE = { "EVENT_DONE", "When trigger has run, it will be marked up.", IS_UP, NULL, NULL, NULL };

/*
 * When a event has failed.
 */
a_state_h EVENT_FAILED = { "EVENT_FAILED", "When trigger has failed." };

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
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_service_type_register(&TYPE_EVENT);

	initng_process_db_ptype_register(&RUN_EVENT);

	initng_active_state_register(&EVENT_WAITING);
	initng_active_state_register(&EVENT_RUNNING);
	initng_active_state_register(&EVENT_FAILED);
	initng_active_state_register(&EVENT_DONE);

	initng_service_data_type_register(&TRIGGER);
	initng_service_data_type_register(&AUTO_RESET);

	initng_event_type_register(&EXTRN_EVENT);
	initng_event_hook_register(&EXTRN_EVENT, &handle_event);

	initng_plugin_hook_register(&g.IS_CHANGE, 99, &event_triggerer);

	return (TRUE);
}

void module_unload(void)
{
	D_("module_unload();\n");

	initng_service_type_unregister(&TYPE_EVENT);

	initng_process_db_ptype_unregister(&RUN_EVENT);

	initng_plugin_hook_unregister(&g.IS_CHANGE, &event_triggerer);

	initng_active_state_unregister(&EVENT_WAITING);
	initng_active_state_unregister(&EVENT_RUNNING);
	initng_active_state_unregister(&EVENT_FAILED);
	initng_active_state_unregister(&EVENT_DONE);

	initng_service_data_type_unregister(&TRIGGER);
	initng_service_data_type_unregister(&AUTO_RESET);

	initng_event_hook_unregister(&EXTRN_EVENT, &handle_event);
	initng_event_type_unregister(&EXTRN_EVENT);
}

/*
 * ############################################################################
 * #                         STATE_FUNCTIONS                                  #
 * ############################################################################
 */

static int event_triggerer(active_db_h * service)
{
	s_extrn_event event;
	const void * eventp = &event;

	s_data *itt = NULL;
	const char *tmp = NULL;
	char *fixed;

	assert(service);
	assert(service->name);

	/* if service is up */
	if (IS_UP(service) && (g.sys_state != STATE_STOPPING))
	{
		event.event_type = &EXTRN_EVENT;
		event.triggerer = service->name;

		/* get the trigger strings */
		while ((tmp = get_next_string(&TRIGGER, service, &itt)))
		{
			/* fix $vars */
			fixed = fix_variables(tmp, service);

			event.target = fixed;
			initng_event_send((s_event *) eventp);

			fix_free(fixed, tmp);
		}
	}

	return (TRUE);
}


/* to do when event is done */
static void handle_event_leave(active_db_h * service, process_h * process)
{
	if (get_int(&AUTO_RESET, service))
	{
		initng_common_mark_service(service, &EVENT_WAITING);
	}
	else
	{
		initng_common_mark_service(service, &EVENT_DONE);
	}
}

static void handle_event(s_event * extrn_event)
{
	s_extrn_event * event = (s_extrn_event *) extrn_event;
	active_db_h * target;

	if (!(target = initng_active_db_find_by_exact_name(event->target)))
	{
		F_("Target service not found\n");
		return;
	}

	if (target->type != &TYPE_EVENT)
	{
		F_("Target service is not event type\n");
		return;
	}

	if (!IS_MARK(target, &EVENT_WAITING))
	{
		F_("Target service has been triggered already\n");
		return;
	}

	initng_common_mark_service(target, &EVENT_RUNNING);

	switch (initng_execute_launch(target, &RUN_EVENT, NULL))
	{
		case FALSE:
			F_("Did not find a run_event entry to run\n");
			initng_common_mark_service(target, &EVENT_FAILED);
			return;
		case FAIL:
			F_("Could not launch run_event\n");
			initng_common_mark_service(target, &EVENT_FAILED);
			return;
	}
}
