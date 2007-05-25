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


INITNG_PLUGIN_MACRO;

static int cmd_get_pid_of(char *arg);
static int cmd_start_on_new(char *arg);
static int cmd_free_service(char *arg);
static int cmd_restart(char *arg);
static char *cmd_print_uptime(char *arg);
static int cmd_initng_halt(char *arg);
static int cmd_initng_poweroff(char *arg);
static int cmd_initng_reboot(char *arg);
static char *cmd_print_modules(char *arg);
static int cmd_load_module(char *arg);

/* static int cmd_unload_module(char *arg); */
static int cmd_percent_done(char *arg);

static char *cmd_get_depends_on(char *arg);
static char *cmd_get_depends_on_deep(char *arg);
static char *cmd_get_depends_off(char *arg);
static char *cmd_get_depends_off_deep(char *arg);
static int cmd_new_init(char *arg);
static int cmd_run(char *arg);

s_command GET_PID_OF = { 'g', "get_pid_of", INT_COMMAND, ADVANCHED_COMMAND, REQUIRES_OPT,
	{(void *) &cmd_get_pid_of}, "Get pid of service"
};
s_command START_ON_NEW = { 'j', "restart_from", TRUE_OR_FALSE_COMMAND, ADVANCHED_COMMAND,
	REQUIRES_OPT,
	{(void *) &cmd_start_on_new},
	"Stop all services, and start from"
};
s_command FREE_SERVICE = { 'z', "zap", TRUE_OR_FALSE_COMMAND, STANDARD_COMMAND, USES_OPT,
	{(void *) &cmd_free_service},
	"Resets a failed service, so it can be restarted."
};
s_command RESTART_SERVICE = { 'r', "restart", TRUE_OR_FALSE_COMMAND, STANDARD_COMMAND,
	REQUIRES_OPT,
	{(void *) &cmd_restart},
	"Restart service"
};

s_command PRINT_UPTIME = { 'T', "time", STRING_COMMAND, ADVANCHED_COMMAND, REQUIRES_OPT,
	{(void *) &cmd_print_uptime},
	"Print uptime"
};

s_command POWEROFF_INITNG = { '0', "poweroff", TRUE_OR_FALSE_COMMAND, STANDARD_COMMAND, NO_OPT,
	{(void *) &cmd_initng_poweroff},
	"Power off the computer"
};
s_command HALT_INITNG = { '1', "halt", TRUE_OR_FALSE_COMMAND, STANDARD_COMMAND, NO_OPT,
	{(void *) &cmd_initng_halt},
	"Halt the computer"
};
s_command REBOOT_INITNG = { '6', "reboot", TRUE_OR_FALSE_COMMAND, STANDARD_COMMAND, NO_OPT,
	{(void *) &cmd_initng_reboot},
	"Reboot the computer"
};


s_command PRINT_MODULES = { 'm', "print_plugins", STRING_COMMAND, ADVANCHED_COMMAND, NO_OPT,
	{(void *) &cmd_print_modules},
	"Print loaded plugins"
};

s_command LOAD_MODULE = { 'o', "load_module", TRUE_OR_FALSE_COMMAND, ADVANCHED_COMMAND,
	REQUIRES_OPT,
	{(void *) &cmd_load_module},
	"Load Module"
};

/* NOT_SAFE_YET TODO
   s_command UNLOAD_MODULE = { 'w', "unload_module", TRUE_OR_FALSE_COMMAND, ADVANCHED_COMMAND,
   REQUIRES_OPT,
   {(void *) &cmd_unload_module},
   "UnLoad Module"
   }; */

s_command PERCENT_DONE = { 'n', "done", INT_COMMAND, ADVANCHED_COMMAND, NO_OPT,
	{(void *) &cmd_percent_done},
	"Prints percent of system up"
};


s_command DEPENDS_ON = { 'a', "service_dep_on", STRING_COMMAND, ADVANCHED_COMMAND, REQUIRES_OPT,
	{(void *) &cmd_get_depends_on},
	"Print what services me depends on"
};

s_command DEPENDS_ON_DEEP = { 'A', "service_dep_on_deep", STRING_COMMAND, ADVANCHED_COMMAND,
	REQUIRES_OPT,
	{(void *) &cmd_get_depends_on_deep},
	"Print what services me depends on deep"
};

s_command DEPENDS_OFF = { 'b', "service_dep_on_me", STRING_COMMAND, ADVANCHED_COMMAND,
	REQUIRES_OPT,
	{(void *) &cmd_get_depends_off},
	"Print what dependencies that are depending on me"
};

s_command DEPENDS_OFF_DEEP = { 'B', "service_dep_on_me_deep", STRING_COMMAND, ADVANCHED_COMMAND,
	REQUIRES_OPT,
	{(void *) &cmd_get_depends_off_deep},
	"Print what dependencies that are depending on me deep"
};


s_command NEW_INIT = { 'E', "new_init", TRUE_OR_FALSE_COMMAND, HIDDEN_COMMAND, REQUIRES_OPT,
	{(void *) &cmd_new_init},
	"Stops all services, and when its done, launching a new init."
};

s_command RUN = { 'U', "run", TRUE_OR_FALSE_COMMAND, ADVANCHED_COMMAND, REQUIRES_OPT,
	{(void *) &cmd_run},
	"Simply run an exec with specified name, example ngc --run service/test:start"
};

static int cmd_get_pid_of(char *arg)
{
	active_db_h *apt = NULL;
	process_h *process = NULL;

	if (!arg)
		return (-2);


	if (!(apt = initng_active_db_find_in_name(arg)))
		return (-1);

	/* browse all processes */
	while_processes(process, apt)
	{
		/* return the first process found */
		return (process->pid);
	}

	return (-3);
}


static int cmd_start_on_new(char *arg)
{
	if (!arg)
		return (FALSE);

	g.when_out = THEN_RESTART;
	initng_main_set_runlevel(arg);
	initng_handler_stop_all();
	return (TRUE);
}

static int cmd_free_service(char *arg)
{
	active_db_h *apt = NULL;
	active_db_h *safe = NULL;
	int ret = FALSE;

	/* check if we got an service */
	if (arg && (apt = initng_active_db_find_in_name(arg)))
	{
		/* zap found */
		initng_active_db_free(apt);
		ret = TRUE;
	}
	else
	{
		/* zap all that is marked FAIL */
		while_active_db_safe(apt, safe)
		{
			if (IS_FAILED(apt))
			{
				initng_active_db_free(apt);
				ret = TRUE;
			}
		}
	}

	return (ret);
}


static int cmd_restart(char *arg)
{
	active_db_h *apt = NULL;

	if (!arg)
	{
		F_("Must tell the service name to restart.\n");
		return (FALSE);
	}

	apt = initng_active_db_find_in_name(arg);
	if (!apt)
	{
		return (FALSE);
		F_("Service \"%s\" not found.\n", arg);
	}

	D_("removing service data for %s, to make sure .ii file is reloaded!\n",
	   arg);

	D_("Restarting service %s\n", apt->name);
	return (initng_handler_restart_service(apt));
}





static char *cmd_print_uptime(char *arg)
{
	active_db_h *apt = NULL;
	struct timeval now;
	char *string;

	if (!arg)
	{
		return (i_strdup("Please tell me what service to get up-time from."));
	}

	apt = initng_active_db_find_in_name(arg);
	if (!apt)
	{
		string = i_calloc(35 + strlen(arg), sizeof(char));
		sprintf(string, "Service \"%s\" is not found!", arg);
		return (string);
	}

	gettimeofday(&now, NULL);
	{
		string = i_calloc(50, sizeof(char));
		sprintf(string, "Up-time is %ims.\n",
				MS_DIFF(now, apt->time_current_state));
		return (string);
	}
}

static int cmd_initng_reboot(char *arg)
{
	(void) arg;
	g.when_out = THEN_REBOOT;
	initng_handler_stop_all();
	return (TRUE);
}

static int cmd_initng_halt(char *arg)
{
	(void) arg;
	g.when_out = THEN_HALT;
	initng_handler_stop_all();
	return (TRUE);
}

static int cmd_initng_poweroff(char *arg)
{
	(void) arg;
	g.when_out = THEN_POWEROFF;
	initng_handler_stop_all();
	return (TRUE);
}

static char *cmd_print_modules(char *arg)
{
	m_h *mod = NULL;
	size_t string_len = 20;
	char *string = i_calloc(string_len, sizeof(char));

	(void) arg;


	sprintf(string, "modules: \n");

	while_module_db(mod)
	{
		/* Increase buffer for adding */
		string_len += (strlen(mod->module_name) +
					   strlen(mod->module_filename) + 40);
		string = i_realloc(string, string_len * sizeof(char));
		strcat(string, "  * ");
		strcat(string, mod->module_name);

		/* make sure its 30 chars */
		{
			int i;

			for (i = strlen(mod->module_name); i < 30; i++)
				strcat(string, " ");
		}
		strcat(string, mod->module_filename);
		strcat(string, "\n");
	}

	/* ok, the string lengh is probably a lot bigger, resize it right */
	string_len = strlen(string) + 1;
	string = i_realloc(string, string_len * sizeof(char));

	return (string);
}

static int cmd_load_module(char *arg)
{
	if (!arg)
		return (FALSE);

	/* load the module */
	if (initng_load_module(arg) == NULL)
		return (FALSE);

	return (TRUE);
}

/* This one is not really safe yet
   static int cmd_unload_module(char *arg)
   {
   if (!arg)
   return (FALSE);

   return (initng_unload_module_named(arg));
   } */

static int cmd_percent_done(char *arg)
{
	(void) arg;

	return (initng_active_db_percent_started());
}

static char *cmd_get_depends_on(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	on = initng_active_db_find_in_name(arg);

	if (!on)
		return (i_strdup("Did not find service."));

	mprintf(&string, "The \"%s\" depends on:\n", on->name);

	while_active_db(current)
	{
		/* if that we are watching needs current */
		if (initng_depend(on, current) == TRUE)
		{
			if (current->current_state && current->current_state->state_name)
			{
				mprintf(&string, "  %s\t\t[%s]\n", current->name,
						current->current_state->state_name);
			}
		}
	}
	return (string);
}

static char *cmd_get_depends_on_deep(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	on = initng_active_db_find_in_name(arg);

	if (!on)
		return (i_strdup("Did not find service."));

	mprintf(&string, "The \"%s\" depends on:\n", on->name);

	while_active_db(current)
	{
		/* if that we are watching needs current */
		if (initng_depend_deep(on, current) == TRUE)
		{
			if (current->current_state && current->current_state->state_name)
			{
				mprintf(&string, "  %s\t\t[%s]\n", current->name,
						current->current_state->state_name);
			}
		}
	}
	return (string);
}

static char *cmd_get_depends_off(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	on = initng_active_db_find_in_name(arg);
	if (!on)
		return (i_strdup("Did not find service."));

	mprintf(&string, "The services that depends on \"%s\":\n", on->name);

	while_active_db(current)
	{
		/* if current need the one we are watching */
		if (initng_depend(current, on) == TRUE)
		{
			if (current->current_state && current->current_state->state_name)
			{
				mprintf(&string, "  %s\t\t[%s]\n", current->name,
						current->current_state->state_name);
			}
		}
	}
	return (string);
}

static char *cmd_get_depends_off_deep(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	on = initng_active_db_find_in_name(arg);

	if (!on)
		return (i_strdup("Did not find service."));

	mprintf(&string, "The the services that depends on \"%s\":\n", on->name);

	while_active_db(current)
	{
		/* if current need the one we are watching */
		if (initng_depend_deep(current, on) == TRUE)
		{
			if (current->current_state && current->current_state->state_name)
			{
				mprintf(&string, "  %s\t\t[%s]\n", current->name,
						current->current_state->state_name);
			}
		}
	}
	return (string);
}


static int cmd_new_init(char *arg)
{
	char *new_i;
	size_t i = 0;

	if (!arg)
		return (FALSE);

	new_i = strdup(arg);
	g.new_init = split_delim(new_i, WHITESPACE, &i, 0);
	g.when_out = THEN_NEW_INIT;

	initng_handler_stop_all();
	return (TRUE);
}


ptype_h EXTERN_RUN = { "extern_arg", NULL };
static int cmd_run(char *arg)
{
	char *runtype = NULL;
	char *serv_name = NULL;
	active_db_h *service = NULL;

	/* search for a ':' char in arg */
	runtype = strchr(arg, ':');
	if (!runtype || runtype[0] != ':')
	{
		W_("Bad format: --run \"%s\"\n");
		return (FALSE);
	}

	/* if serv name is less then 2 chars */
	if (runtype - arg - 1 < 2)
	{
		W_("Bad format: --run \"%s\"\n");
		return (FALSE);
	}

	/* skip the ':' char */
	runtype++;

	/* copy serv_name so we can put a '\0' to mark end */
	serv_name = i_strndup(arg, runtype - arg - 1);

	service = initng_active_db_find_by_name(serv_name);
	if (!service)
	{
		F_("Service \"%s\" was not found, trying to run.\n", serv_name);
		free(serv_name);
		return (FALSE);
	}

	/* wont need it anymore */
	free(serv_name);

	if (initng_execute_launch(service, &EXTERN_RUN, runtype) != TRUE)
		return (FALSE);

	/* return happily */
	return (TRUE);
}

int module_init(int api_version)
{
	D_("module_init(stcmd);\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_command_register(&GET_PID_OF);
	initng_command_register(&START_ON_NEW);
	initng_command_register(&FREE_SERVICE);
	initng_command_register(&RESTART_SERVICE);
	initng_command_register(&PRINT_UPTIME);
	if (g.i_am == I_AM_INIT)
	{
		initng_command_register(&REBOOT_INITNG);
		initng_command_register(&POWEROFF_INITNG);
		initng_command_register(&HALT_INITNG);
	}
	initng_command_register(&PRINT_MODULES);
	initng_command_register(&LOAD_MODULE);
	/* initng_command_register(&UNLOAD_MODULE); */
	initng_command_register(&PERCENT_DONE);
	initng_command_register(&DEPENDS_ON);
	initng_command_register(&DEPENDS_ON_DEEP);
	initng_command_register(&DEPENDS_OFF);
	initng_command_register(&DEPENDS_OFF_DEEP);
	initng_command_register(&NEW_INIT);
	initng_command_register(&RUN);

	D_("libstcmd.so.0.0 loaded!\n");
	return (TRUE);
}


void module_unload(void)
{
	D_("module_unload(stcmd);\n");

	initng_command_unregister(&GET_PID_OF);
	initng_command_unregister(&START_ON_NEW);
	initng_command_unregister(&FREE_SERVICE);
	initng_command_unregister(&RESTART_SERVICE);
	initng_command_unregister(&PRINT_UPTIME);
	if (g.i_am == I_AM_INIT)
	{
		initng_command_unregister(&REBOOT_INITNG);
		initng_command_unregister(&POWEROFF_INITNG);
		initng_command_unregister(&HALT_INITNG);
	}
	initng_command_unregister(&PRINT_MODULES);
	initng_command_unregister(&LOAD_MODULE);
	/* initng_command_unregister(&UNLOAD_MODULE); */
	initng_command_unregister(&PERCENT_DONE);
	initng_command_unregister(&DEPENDS_ON);
	initng_command_unregister(&DEPENDS_ON_DEEP);
	initng_command_unregister(&DEPENDS_OFF);
	initng_command_unregister(&DEPENDS_OFF_DEEP);
	initng_command_unregister(&NEW_INIT);
	initng_command_unregister(&RUN);

	D_("libstcmd.so.0.0 unloaded!\n");

}