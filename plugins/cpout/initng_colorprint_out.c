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
#include <initng_handler.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include "initng_colorprint_out.h"

#include <initng_global.h>
#include <initng_string_tools.h>
#include <initng_active_db.h>
#include <initng_plugin_hook.h>
#include <initng_static_states.h>
#include <initng_toolbox.h>

INITNG_PLUGIN_MACRO;

#define CPE C_RED " %3i%% " C_OFF C_BLUE "%s" C_OFF MOVE_TO_R
#define CP C_RED "      " C_OFF C_BLUE "%s" C_OFF MOVE_TO_R
#define PE " %3i%% %s"
#define P "      %s"

active_db_h *lastservice;
ptype_h *last_ptype;

FILE *output;
int color = FALSE;

#define cprintf(...) fprintf(output, ## __VA_ARGS__ )

/* when this is set, initng wont print with cpout when system is up */
int quiet_when_up = FALSE;

static void clear_lastserv(void)
{
	if (lastservice)
		cprintf("\n");
	lastservice = NULL;
	last_ptype = NULL;
}


static void out_service_done(active_db_h * s)
{
	int t;

	assert(s);
	assert(s->name);

	t = MS_DIFF(s->time_current_state, s->time_last_state);

	clear_lastserv();
	if (t > 1)
	{
		if (g.sys_state == STATE_STARTING)
		{
			if (color)
				cprintf(CPE "\t[" C_GREEN "done" C_OFF
						"]  \t( done in %ims )\n",
						initng_active_db_percent_started(), s->name, t);
			else
				cprintf(PE "\t[done]  \t( done in %ims )\n",
						initng_active_db_percent_started(), s->name, t);
		}
		else
		{
			if (color)
				cprintf(CP "\t[" C_GREEN "done" C_OFF
						"]  \t( done in %ims )\n", s->name, t);
			else
				cprintf(P "\t[done]  \t( done in %ims )\n", s->name, t);
		}
	}
	else
	{
		if (g.sys_state == STATE_STARTING)
		{
			if (color)
				cprintf(CPE "\t[" C_GREEN "done" C_OFF "]\n",
						initng_active_db_percent_started(), s->name);
			else
				cprintf(PE "\t[done]\n",
						initng_active_db_percent_started(), s->name);
		}
		else
		{
			if (color)
				cprintf(CP "\t[" C_GREEN "done" C_OFF "]\n", s->name);
			else
				cprintf(P "\t[done]\n", s->name);
		}
	}

	/* Make sure its outputed */
	fflush(output);
}

static void opt_service_stop_p(active_db_h * s, const char *is)
{
	int t;

	assert(s);
	assert(s->name);

	t = MS_DIFF(s->time_current_state, s->time_last_state);

	clear_lastserv();
	if (t > 1)
	{
		if (g.sys_state == STATE_STOPPING)
		{
			if (color)
				cprintf(CPE "\t[" C_GREEN "%s" C_OFF
						"]\t( %s in %ims )\n",
						initng_active_db_percent_stopped(), s->name, is, is,
						t);
			else
				cprintf(PE "\t[%s]\t( %s in %ims )\n",
						initng_active_db_percent_stopped(), s->name, is, is,
						t);

		}
		else
		{
			if (color)
				cprintf(CP "\t[" C_GREEN "%s" C_OFF
						"]\t( %s in %ims )\n", s->name, is, is, t);
			else
				cprintf(P "\t[%s]\t( %s in %ims )\n", s->name, is, is, t);
		}
	}
	else
	{
		if (g.sys_state == STATE_STOPPING)
		{
			if (color)
				cprintf(CPE "\t[" C_GREEN "%s" C_OFF "]\n",
						initng_active_db_percent_stopped(), s->name, is);
			else
				cprintf(PE "\t[%s]\n",
						initng_active_db_percent_stopped(), s->name, is);
		}
		else
		{
			if (color)
				cprintf(CP "\t[" C_GREEN "%s" C_OFF "]\n", s->name, is);
			else
				cprintf(P "\t[%s]\n", s->name, is);
		}
	}

	/* Make sure its outputed */
	fflush(output);
}


static int print_output(active_db_h * service)
{
	assert(service);
	assert(service->name);

	/* dont print hidden services */
	if (service->type && service->type->hidden == TRUE)
		return (TRUE);

	/* if quiet_when_up and system up, dont print anything */
	if (quiet_when_up && g.sys_state == STATE_UP)
		return (TRUE);

	if (IS_DOWN(service))
	{
		opt_service_stop_p(service, "stopped");
		return (TRUE);
	}

	if (IS_STARTING(service))
	{
		/* if we print this on boot, we clutter up the screen too much */
		if (g.sys_state == STATE_STARTING)
			return (TRUE);
		clear_lastserv();
		if (color)
			cprintf(CP "\t[" C_GREEN "starting" C_OFF "]\n", service->name);
		else
			cprintf(P "\t[starting]\n", service->name);
		fflush(output);
		return (TRUE);
	}

	if (IS_UP(service))
	{
		int t;
		process_h *process = initng_process_db_get_by_name("daemon", service);

		if (!process)
		{
			out_service_done(service);
			return (TRUE);
		}
		clear_lastserv();
		t = initng_active_db_percent_started();

		if (t > 1 && g.sys_state == STATE_STARTING)
		{
			if (color)
				cprintf(CPE "\t[" C_GREEN "started" C_OFF "]\t( pid: %i )\n",
						t, service->name, process->pid);
			else
				cprintf(PE "\t[started]\t( pid: %i )\n",
						t, service->name, process->pid);
		}
		else
		{
			if (color)
				cprintf(CP "\t[" C_GREEN "started" C_OFF "]\t( pid: %i )\n",
						service->name, process->pid);
			else
				cprintf(P "\t[started]\t( pid: %i )\n",
						service->name, process->pid);
		}
		fflush(output);
		return (TRUE);
	}

	if (IS_STOPPING(service))
	{
		/*
		 * don't prompt that we are stopping a service, if system is shutting down, i 
		 * do think that the user is aware about this.
		 */
		if (g.sys_state == STATE_STOPPING)
			return (TRUE);

		clear_lastserv();
		if (color)
			cprintf(CP "\t[" C_GREEN "stopping" C_OFF "]\n", service->name);
		else
			cprintf(P "\t[stopping]\n", service->name);
		fflush(output);
		return (TRUE);
	}

	/* Print all states, that is a failure state */
	if (IS_FAILED(service))
	{
		clear_lastserv();
		if (color)
			cprintf(CP "\t[" C_RED "%s" C_OFF "]\n", service->name,
					service->current_state->state_name);
		else
			cprintf(P "\t[%s]\n", service->name,
					service->current_state->state_name);
	}

	fflush(output);
	return (TRUE);
}

static void print_system_state(h_sys_state state)
{

	switch (state)
	{
		case STATE_STARTING:
			clear_lastserv();
			cprintf("\tSystem is starting up!\n\n");
			break;
		case STATE_STOPPING:
			clear_lastserv();
			cprintf("\tSystem is going down!\n\n");
			break;
		case STATE_ASE:
			clear_lastserv();
			cprintf("\tLast process exited!\n\n");
			break;
		case STATE_EXIT:
			clear_lastserv();
			cprintf("\tInitng exiting!\n\n");
			break;
		case STATE_RESTART:
			clear_lastserv();
			cprintf("\tInitng restarting!\n\n");
			break;
		case STATE_REBOOT:
			clear_lastserv();
			cprintf("\n\tYour system will now REBOOT!\n");
			break;
		case STATE_HALT:
			clear_lastserv();
			cprintf("\n\tYour system will now HALT!\n");
			break;
		case STATE_POWEROFF:
			clear_lastserv();
			cprintf("\n\tYour system will now POWER_OFF!\n");
			break;
		case STATE_UP:
			{
				struct timeval now;

				/* Dont print this */
				if (quiet_when_up)
					return;

				/* get runlevel */
				active_db_h *runl = initng_active_db_find_by_name(g.runlevel);

				/* this is not critical, its youst fr runlevel up in info */
				if (!runl)
				{
					D_("Runlevel %s not found.\n", g.runlevel);
					return;
				}

				gettimeofday(&now, NULL);

				clear_lastserv();
				cprintf("\n\n\trunlevel \"%s\" up in, %ims.\n\n",
						g.runlevel, MS_DIFF(now, runl->last_rought_time));

				break;
			}
		default:
			break;
	}

	fflush(output);
	D_("print_system_state(): new system state: %i\n", state);
}

static int print_program_output(active_db_h * service, process_h * x,
								pipe_h * pi, char *buffer_pos)
{
	/*
	   TODO here:
	   we should have an internal list of services and "our" current position in them.

	   That way when this function is called we can print every full line from plugin_pos.
	   This way fsck will look nice, along with an "internal" database of write positions we can cache data so we print every 5 seconds or on int forceflush.
	 */
	int i = 0;

	assert(service);
	assert(service->name);
	assert(x);
	S_;

	/* if quiet_when_up and system up, dont print anything */
	if (quiet_when_up && g.sys_state == STATE_UP)
		return (TRUE);

	D_(" from service \"%s\"\n", service->name);
	/*
	   cprintf("buffer_pos: %i\n", buffer_pos);
	   cprintf("datalen: %i\n", datalen);
	   cprintf("Buffer: \n################\n%s\n##########\n\n",x->buffer);
	 */
	/* a first while loop that sorts out crap */
	while (buffer_pos[i] != '\0')
	{
		/*  remove lines with " [2]  Done " that bash generates. */
		if (buffer_pos[i] == '[' && buffer_pos[i + 2] == ']')
		{
			/* jump to next line */
			while (buffer_pos[i] && buffer_pos[i] != '\n')
				i++;
		}

		/* if there are stupid tokens, go to next char, and run while again. */
		if (buffer_pos[i] == ' ' || buffer_pos[i] == '\n'
			|| buffer_pos[i] == '\t')
		{
			i++;
			continue;
		}

		/* else break */
		break;
	}

	/* Make sure that there is anything left to write */
	if (strlen(&buffer_pos[i]) < 2)
	{
		/* its okay anyway */
		return (TRUE);
	}

	if (lastservice != service && last_ptype != x->pt)
	{
		clear_lastserv();
		if (color)
			cprintf("\n" C_CYAN " %s %s:" C_OFF, service->name, x->pt->name);
		else
			cprintf("\n %s %s:", service->name, x->pt->name);
		/* print our special indented newline */
		putc('\n', output);
		putc(' ', output);
		putc(' ', output);
		lastservice = service;
		last_ptype = x->pt;
	}
	else
	{
		D_("Lastservice == service, won't print header.\n");
	}



	/* while buffer lasts */
	while (buffer_pos[i] != '\0')
	{
		/*  remove lines with " [2]  Done " that bash generates. */
		if (buffer_pos[i] == '[' && buffer_pos[i + 2] == ']')
		{
			while (buffer_pos[i] && buffer_pos[i] != '\n')
				i++;
		}

		/* if this are a newline */
		if (buffer_pos[i] == '\n')
		{
			/* print our special indented newline instead */
			putc('\n', output);
			putc(' ', output);
			putc(' ', output);
			i++;
			/* skip spaces, on newline. */
			while (buffer_pos[i]
				   && (buffer_pos[i] == ' ' || buffer_pos[i] == '\t'))
				i++;
			continue;
		}

		/* ok, now put the char, and go to next. */
		putc(buffer_pos[i], output);
		i++;
	}

	/* flush any buffered output to the screen */
	fflush(stdout);
	return (TRUE);
}


static int cp_print_error(e_mt mt, const char *file, const char *func,
						  int line, const char *format, va_list arg)
{
	struct tm *ts;
	time_t t;

	switch (mt)
	{
		case MSG_FAIL:
		case MSG_WARN:
			t = time(0);
			ts = localtime(&t);
#ifdef DEBUG
			fprintf(output, "\n\n ** \"%s\", %s()  line:%i:\n", file, func,
					line);
#endif
			fprintf(output, " %.2i:%.2i:%.2i -- %s:\t", ts->tm_hour,
					ts->tm_min, ts->tm_sec, mt == MSG_FAIL ? "FAIL" : "WARN");
			vfprintf(output, format, arg);
			break;
		default:
			vfprintf(output, format, arg);
			break;
	}

	/* make sure it reach screen */
	fflush(output);
	return (TRUE);
}

int module_init(int api_version)
{
	int i;

	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	/* only add this module if this is a real init */
	if (g.i_am != I_AM_INIT && g.i_am != I_AM_FAKE_INIT)
		return (TRUE);

	output = stdout;

	/* check if output is specified */
	for (i = 0; g.Argv[i]; i++)
	{
		if (!g.Argv[i])
			continue;

		/* check for cpout_console */
		if ((strlen(g.Argv[i]) > 16) &&
			(strstr(g.Argv[i], "cpout_console=")
			 || strstr(g.Argv[i], "cpout_console:")))
		{
			printf("cpout_console=%s\n", &g.Argv[i][14]);
			output = fopen(&g.Argv[i][14], "w");
			continue;
		}

		/* check for cpout_nocolors */
		if (strcmp("cpout_nocolors", g.Argv[i]) == 0)
			color = -1;

		/* check for quiet_when_up */
		if (strstr(g.Argv[i], "quiet_when_up"))
			quiet_when_up = TRUE;

	}

	/* user might fore that we dont use any color */
#ifndef FORCE_NOCOLOR
	/* enable color only on terminals */
	if (isatty(fileno(output)))
		color += 1;							/* if this is -1 from abow, this will be zero now */
	else
#endif
		color = 0;

	if (color)
	{
		cprintf("\n" C_BLUE "\tNext Generation Init version ( %s )" C_OFF,
				INITNG_VERSION);
		cprintf("\n" C_GREEN "\thttp://www.initng.org\n" C_OFF);
	}
	else
	{
		cprintf("\n\tNext Generation Init version ( %s )", INITNG_VERSION);
		cprintf("\n\thttp://www.initng.org\n");
	}
	cprintf("\tAuthor: Jimmy Wennlund <jimmy.wennlund@gmail.com>\n");
	cprintf("\tIf you find initng useful, please consider a small donation.\n\n");
	fflush(output);
	D_("module_init();\n");
	lastservice = NULL;
	initng_plugin_hook_register(&g.ERR_MSG, 10, &cp_print_error);
	initng_plugin_hook_register(&g.IS_CHANGE, 80, &print_output);
	initng_plugin_hook_register(&g.SWATCHERS, 80, &print_system_state);
	initng_plugin_hook_register(&g.BUFFER_WATCHER, 50, &print_program_output);
	return (TRUE);
}

void module_unload(void)
{
	D_("color_out: module_unload();\n");
	if (g.i_am != I_AM_INIT && g.i_am != I_AM_FAKE_INIT)
		return;


	initng_plugin_hook_unregister(&g.IS_CHANGE, &print_output);
	initng_plugin_hook_unregister(&g.SWATCHERS, &print_system_state);
	initng_plugin_hook_unregister(&g.BUFFER_WATCHER, &print_program_output);
	initng_plugin_hook_unregister(&g.ERR_MSG, &cp_print_error);
	cprintf("  Goodbye\n");
	fflush(output);

	/* close output fifo */
	if (output != stdout)
		fclose(output);
}
