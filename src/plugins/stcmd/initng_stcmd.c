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

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
}

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
static int cmd_signal(char *arg);

s_command GET_PID_OF = {
	.id = 'g',
	.long_id = "get_pid_of",
	.com_type = INT_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_get_pid_of},
	.description = "Get pid of service"
};

s_command START_ON_NEW = {
	.id = 'j',
	.long_id = "restart_from",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_start_on_new},
	.description = "Stop all services, and start from"
};

s_command FREE_SERVICE = {
	.id = 'z',
	.long_id = "zap",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = USES_OPT,
	.u = {(void *)&cmd_free_service},
	.description = "Resets a failed service, so it can be restarted."
};

s_command RESTART_SERVICE = {
	.id = 'r',
	.long_id = "restart",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_restart},
	.description = "Restart service"
};

s_command PRINT_UPTIME = {
	.id = 'T',
	.long_id = "time",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_print_uptime},
	.description = "Print uptime"
};

s_command POWEROFF_INITNG = {
	.id = '0',
	.long_id = "poweroff",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_initng_poweroff},
	.description = "Power off the computer"
};

s_command HALT_INITNG = {
	.id = '1',
	.long_id = "halt",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_initng_halt},
	.description = "Halt the computer"
};

s_command REBOOT_INITNG = {
	.id = '6',
	.long_id = "reboot",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_initng_reboot},
	.description = "Reboot the computer"
};

s_command PRINT_MODULES = {
	.id = 'm',
	.long_id = "print_plugins",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_print_modules},
	.description = "Print loaded plugins"
};

s_command LOAD_MODULE = {
	.id = 'o',
	.long_id = "load_module",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_load_module},
	.description = "Load Module"
};

#if 0				/* NOT_SAFE_YET TODO */
s_command UNLOAD_MODULE = {
	.id = 'w',
	.long_id = "unload_module",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_unload_module},
	.description = "UnLoad Module"
};
#endif

s_command PERCENT_DONE = {
	.id = 'n',
	.long_id = "done",
	.com_type = INT_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_percent_done},
	.description = "Prints percent of system up"
};

s_command DEPENDS_ON = {
	.id = 'a',
	.long_id = "service_dep_on",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_get_depends_on},
	.description = "Print what services me depends on"
};

s_command DEPENDS_ON_DEEP = {
	.id = 'A',
	.long_id = "service_dep_on_deep",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_get_depends_on_deep},
	.description = "Print what services me depends on deep"
};

s_command DEPENDS_OFF = {
	.id = 'b',
	.long_id = "service_dep_on_me",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_get_depends_off},
	.description = "Print what dependencies that are depending on me"
};

s_command DEPENDS_OFF_DEEP = {
	.id = 'B',
	.long_id = "service_dep_on_me_deep",
	.com_type = STRING_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_get_depends_off_deep},
	.description = "Print what dependencies that are depending on me deep"
};

s_command NEW_INIT = {
	.id = 'E',
	.long_id = "new_init",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = HIDDEN_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_new_init},
	.description = "Stops all services, and when its done, launching a "
	    "new init."
};

s_command RUN = {
	.id = 'U',
	.long_id = "run",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = ADVANCHED_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_run},
	.description = "Simply run an exec with specified name, example "
	    "ngc --run service/test:start"
};

s_command SIGNAL = {
	.id = 'l',
	.long_id = "signal",
	.com_type = TRUE_OR_FALSE_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = REQUIRES_OPT,
	.u = {(void *)&cmd_signal},
	.description = "Send a signal to the specified service."
};

static int cmd_signal(char *arg)
{
	char *sname;
	int sig;
	int pid;

	if (!(sname = strrchr(arg, ':')) || (pid = cmd_get_pid_of(arg)) < 2)
		return FALSE;

	sig = strtol(++sname, NULL, 0);

	if (sig < 1 || kill(pid, sig))
		return FALSE;

	return TRUE;
}

static int cmd_get_pid_of(char *arg)
{
	active_db_h *apt = NULL;
	process_h *process = NULL;

	if (!arg)
		return -2;

	if (!(apt = initng_active_db_find_in_name(arg)))
		return -1;

	/* browse all processes */
	while_processes(process, apt) {
		/* return the first process found */
		return process->pid;
	}

	return -3;
}

static int cmd_start_on_new(char *arg)
{
	if (!arg)
		return FALSE;

	g.when_out = THEN_RESTART;
	initng_main_set_runlevel(arg);
	initng_handler_stop_all();
	return TRUE;
}

static int cmd_free_service(char *arg)
{
	active_db_h *apt = NULL;
	active_db_h *safe = NULL;
	int ret = FALSE;

	/* check if we got an service */
	if (arg && (apt = initng_active_db_find_in_name(arg))) {
		/* zap found */
		initng_active_db_free(apt);
		ret = TRUE;
	} else {
		/* zap all that is marked FAIL */
		while_active_db_safe(apt, safe) {
			if (IS_FAILED(apt)) {
				initng_active_db_free(apt);
				ret = TRUE;
			}
		}
	}

	return ret;
}

static int cmd_restart(char *arg)
{
	active_db_h *apt = NULL;

	if (!arg) {
		F_("Must tell the service name to restart.\n");
		return FALSE;
	}

	apt = initng_active_db_find_in_name(arg);
	if (!apt) {
		return FALSE;
		F_("Service \"%s\" not found.\n", arg);
	}

	D_("removing service data for %s, to make sure .ii file is "
	   "reloaded!\n", arg);

	D_("Restarting service %s\n", apt->name);
	return (initng_handler_restart_service(apt));
}

static char *cmd_print_uptime(char *arg)
{
	active_db_h *apt = NULL;
	struct timeval now;
	char *string;

	if (!arg) {
		return initng_toolbox_strdup("Please tell me what service "
					     "to get up-time from.");
	}

	apt = initng_active_db_find_in_name(arg);
	if (!apt) {
		string = initng_toolbox_calloc(35 + strlen(arg), 1);
		sprintf(string, "Service \"%s\" is not found!", arg);
		return string;
	}

	gettimeofday(&now, NULL);

	{
		string = initng_toolbox_calloc(50, 1);
		sprintf(string, "Up-time is %ims.\n",
			MS_DIFF(now, apt->time_current_state));
		return string;
	}
}

static int cmd_initng_reboot(char *arg)
{
	(void)arg;
	g.when_out = THEN_REBOOT;
	initng_handler_stop_all();
	return TRUE;
}

static int cmd_initng_halt(char *arg)
{
	(void)arg;
	g.when_out = THEN_HALT;
	initng_handler_stop_all();
	return TRUE;
}

static int cmd_initng_poweroff(char *arg)
{
	(void)arg;
	g.when_out = THEN_POWEROFF;
	initng_handler_stop_all();
	return TRUE;
}

static char *cmd_print_modules(char *arg)
{
	m_h *mod = NULL;
	size_t string_len = 20;
	char *string = initng_toolbox_calloc(string_len, 1);

	(void)arg;

	string_len = sprintf(string, "modules: \n");

	while_module_db(mod) {
		int namelen;
		char *append_at;

		namelen = strlen(mod->name);
		append_at = string + string_len;
		/* Increase buffer for adding */
		string_len += 6 + strlen(mod->path)
				+ namelen < 30 ? 30 : namelen;

		string = initng_toolbox_realloc(string, string_len);
		sprintf(append_at, "  * %-30s %s\n", mod->name, mod->path);
	}

	return string;
}

static int cmd_load_module(char *arg)
{
	if (!arg)
		return FALSE;

	/* load the module */
	if (initng_module_load(arg) == NULL)
		return FALSE;

	return TRUE;
}

#if 0				/* This one is not really safe yet */
static int cmd_unload_module(char *arg)
{
	if (!arg)
		return FALSE;

	return initng_unload_module_named(arg);
}
#endif

static int cmd_percent_done(char *arg)
{
	(void)arg;

	return initng_active_db_percent_started();
}

static char *cmd_get_depends_on(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	if (!(on = initng_active_db_find_in_name(arg)))
		return initng_toolbox_strdup("Did not find service.");

	initng_string_mprintf(&string, "The \"%s\" depends on:\n", on->name);

	while_active_db(current) {
		/* if that we are watching needs current */
		if (initng_depend(on, current) == TRUE) {
			if (current->current_state &&
			    current->current_state->name) {
				initng_string_mprintf(&string, "  %s\t\t[%s]\n",
					current->name,
					current->current_state->name);
			}
		}
	}

	return string;
}

static char *cmd_get_depends_on_deep(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	if (!(on = initng_active_db_find_in_name(arg)))
		return initng_toolbox_strdup("Did not find service.");

	initng_string_mprintf(&string, "The \"%s\" depends on:\n", on->name);

	while_active_db(current) {
		/* if that we are watching needs current */
		if (initng_depend_deep(on, current) == TRUE) {
			if (current->current_state &&
			    current->current_state->name) {
				initng_string_mprintf(&string, "  %s\t\t[%s]\n",
					current->name,
					current->current_state->name);
			}
		}
	}

	return string;
}

static char *cmd_get_depends_off(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	if (!(on = initng_active_db_find_in_name(arg)))
		return initng_toolbox_strdup("Did not find service.");

	initng_string_mprintf(&string, "The services that depends on \"%s\":\n", on->name);

	while_active_db(current) {
		/* if current need the one we are watching */
		if (initng_depend(current, on) == TRUE) {
			if (current->current_state &&
			    current->current_state->name) {
				initng_string_mprintf(&string, "  %s\t\t[%s]\n",
					current->name,
					current->current_state->name);
			}
		}
	}

	return string;
}

static char *cmd_get_depends_off_deep(char *arg)
{
	char *string = NULL;
	active_db_h *current = NULL;
	active_db_h *on = NULL;

	if (!(on = initng_active_db_find_in_name(arg)))
		return (initng_toolbox_strdup("Did not find service."));

	initng_string_mprintf(&string, "The the services that depends on \"%s\":\n",
		on->name);

	while_active_db(current) {
		/* if current need the one we are watching */
		if (initng_depend_deep(current, on) == TRUE) {
			if (current->current_state &&
			    current->current_state->name) {
				initng_string_mprintf(&string, "  %s\t\t[%s]\n",
					current->name,
					current->current_state->name);
			}
		}
	}

	return string;
}

static int cmd_new_init(char *arg)
{
	char *new_i;

	if (!arg)
		return FALSE;

	new_i = strdup(arg);
	g.new_init = initng_string_split_delim(new_i, WHITESPACE, NULL);
	g.when_out = THEN_NEW_INIT;

	initng_handler_stop_all();
	return TRUE;
}

ptype_h EXTERN_RUN = { "extern_arg", NULL };

static int cmd_run(char *arg)
{
	char *runtype = NULL;
	char *serv_name = NULL;
	active_db_h *service = NULL;

	/* search for a ':' char in arg */
	runtype = strchr(arg, ':');
	if (!runtype || runtype[0] != ':') {
		W_("Bad format: --run \"%s\"\n");
		return FALSE;
	}

	/* if serv name is less then 2 chars */
	if (runtype - arg < 3) {
		W_("Bad format: --run \"%s\"\n");
		return FALSE;
	}

	/* skip the ':' char */
	runtype++;

	/* copy serv_name so we can put a '\0' to mark end */
	serv_name = initng_toolbox_strndup(arg, runtype - arg - 1);

	service = initng_active_db_find_by_name(serv_name);
	if (!service) {
		F_("Service \"%s\" was not found, trying to run.\n", serv_name);
		free(serv_name);
		return FALSE;
	}

	/* wont need it anymore */
	free(serv_name);

	if (initng_execute_launch(service, &EXTERN_RUN, runtype) != TRUE)
		return FALSE;

	/* return happily */
	return TRUE;
}

int module_init(int api_version)
{
	D_("module_init(stcmd);\n");
	if (api_version != API_VERSION) {
		F_("This module is compiled for api_version %i version and "
		   "initng is compiled on %i version, won't load this "
		   "module!\n", API_VERSION, api_version);
		return FALSE;
	}

	initng_command_register(&GET_PID_OF);
	initng_command_register(&START_ON_NEW);
	initng_command_register(&FREE_SERVICE);
	initng_command_register(&RESTART_SERVICE);
	initng_command_register(&PRINT_UPTIME);

	initng_command_register(&REBOOT_INITNG);
	initng_command_register(&POWEROFF_INITNG);
	initng_command_register(&HALT_INITNG);

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
	return TRUE;
}

void module_unload(void)
{
	D_("module_unload(stcmd);\n");

	initng_command_unregister(&GET_PID_OF);
	initng_command_unregister(&START_ON_NEW);
	initng_command_unregister(&FREE_SERVICE);
	initng_command_unregister(&RESTART_SERVICE);
	initng_command_unregister(&PRINT_UPTIME);

	initng_command_unregister(&REBOOT_INITNG);
	initng_command_unregister(&POWEROFF_INITNG);
	initng_command_unregister(&HALT_INITNG);

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
