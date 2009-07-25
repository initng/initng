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
#include <stdlib.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

#include "print_service.h"

static char *cmd_print_fds(char *arg);
static int cmd_initng_quit(char *arg);

#ifdef DEBUG
static int cmd_toggle_verbose(char *arg);
static int cmd_add_verbose(char *arg);
static int cmd_del_verbose(char *arg);
#endif

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
};


s_command LIST_FDS = {
	.id = 'I',
	.long_id = "list_filedescriptors",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = USES_OPT,
	.u = {(void *)&cmd_print_fds},
	.description = "Print all open filedescriptors initng have."
};

s_command QUIT_INITNG = {
	.id = 'q',
	.long_id = "quit",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_initng_quit},
	.description = "Quits initng"
};

s_command PRINT_ACTIVE_DB = {
	.id = 'p',
	.long_id = "print_active_db",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = USES_OPT,
	.u = {(void *)&active_db_print_all},
	.description = "Print active_db"
};

#ifdef DEBUG
s_command TOGGLE_VERBOSE = {
	.id = 'v',
	.long_id = "verbose",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_toggle_verbose},
	.description = "Toggle the verbose flag - ONLY FOR DEBUGGING"
};

s_command ADD_VERBOSE = {
	.id = 'i',
	.long_id = "add_verbose",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_add_verbose},
	.description = "Add string to watch for to make initng verbose - "
	    "ONLY FOR DEBUGGING"
};

s_command DEL_VERBOSE = {
	.id = 'k',
	.long_id = "del_verbose",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_del_verbose},
	.description = "Del string to watch for to make initng verbose - "
	    "ONLY FOR DEBUGGING"
};
#endif

static char *cmd_print_fds(char *arg)
{
	char *string = NULL;
	active_db_h *currentA;
	process_h *currentP;
	pipe_h *current_pipe;
	int i;

	{
		s_event event;
		s_event_fd_watcher_data data;

		event.event_type = &EVENT_FD_WATCHER;
		event.data = &data;
		data.action = FDW_ACTION_DEBUG;
		data.debug_find_what = arg;
		data.debug_out = &string;

		initng_event_send(&event);
	}

	for (i = 0; i < 1024; i++) {
		currentA = NULL;

		/* for every service */
		while_active_db(currentA) {
			/* if argument was set, only print matching */
			if (!arg || initng_string_match(currentA->name, arg)) {
				/* for every process */
				currentP = NULL;
				while_processes(currentP, currentA) {
					/* for every pipe */
					current_pipe = NULL;
					while_pipes(current_pipe, currentP) {
						/* if matching */
						if (current_pipe->pipe[0] == i
						    || current_pipe->pipe[1] ==
						    i) {
							/* PRINT */
							initng_string_mprintf(&string,
								" %i: Used "
								"service: %s, "
								"process: "
								"%s\n", i,
								currentA->name,
								currentP->pt->
								name);
						}
					}
				}
			}
		}

		/*initng_string_mprintf(&string, " %i:\n", i); */
	}

	return string;
}

static int cmd_initng_quit(char *arg)
{
	(void)arg;
	g.when_out = THEN_QUIT;
	initng_handler_stop_all();

	return TRUE;
}

#ifdef DEBUG

static int cmd_toggle_verbose(char *arg)
{
	(void)arg;
	switch (g.verbose) {
	case 0:
		g.verbose = 1;
		break;

	case 1:
		g.verbose = 0;
		break;

	case 2:
		g.verbose = 3;
		break;

	case 3:
		g.verbose = 2;
		break;

	default:
		g.verbose = 0;
		W_("Unknown verbose id %i\n", g.verbose);
		break;
	}

	return (g.verbose);
}

static int cmd_add_verbose(char *arg)
{
	if (!arg)
		return FALSE;

	return initng_error_verbose_add(arg);
}

static int cmd_del_verbose(char *arg)
{
	if (!arg)
		return FALSE;

	return initng_error_verbose_del(arg);
}
#endif

int module_init(void)
{
	initng_command_register(&LIST_FDS);
	initng_command_register(&PRINT_ACTIVE_DB);

#ifdef DEBUG
	initng_command_register(&TOGGLE_VERBOSE);
	initng_command_register(&ADD_VERBOSE);
	initng_command_register(&DEL_VERBOSE);
#endif

	return TRUE;
}

void module_unload(void)
{
	initng_command_unregister(&LIST_FDS);
	initng_command_unregister(&PRINT_ACTIVE_DB);
#ifdef DEBUG
	initng_command_unregister(&TOGGLE_VERBOSE);
	initng_command_unregister(&ADD_VERBOSE);
	initng_command_unregister(&DEL_VERBOSE);
#endif
}
