/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2005 neuron <neuron@hollowtube.mine.nu>
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
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <assert.h>

#include <initng_global.h>
#include <initng_process_db.h>
#include <initng_handler.h>
#include <initng_main.h>
#include <initng_active_db.h>
#include <initng_toolbox.h>
#include <initng_plugin_hook.h>
#include <initng_plugin.h>
#include <initng_load_module.h>
#include <initng_static_states.h>
#include <initng_event_hook.h>
#include <initng_static_event_types.h>

#include <initng-paths.h>

#include <utmp.h>
#include "initreq.h"

INITNG_PLUGIN_MACRO;

int utmp_stored = FALSE;

/* static local function here */
static void parse_control_input(f_module_h * mod, e_fdw what);
static void initctl_control_close(void);
static int initctl_control_open(void);
static void makeutmp(int runlevel);
static void initng_reload(void);


f_module_h pipe_fd = { &parse_control_input, FDW_READ, -1 };	/* /dev/initctl */

struct stat st, st2;

#define PIPE_FD    10						/* Fileno of initfifo. */

static void initctl_control_close(void)
{
	if (pipe_fd.fds > 2)
	{
		close(pipe_fd.fds);
		pipe_fd.fds = -1;
	}
}

static int initctl_control_open(void)
{
	D_("initctl control open (%d)\n", pipe_fd.fds);
	/* First, try to create /dev/initctl if not present. */
	if (stat(INIT_FIFO, &st2) < 0 && errno == ENOENT)
		(void) mkfifo(INIT_FIFO, 0600);

	/*
	 *  If /dev/initctl is open, stat the file to see if it
	 *  is still the _same_ inode.
	 */
	if (pipe_fd.fds > 2)
	{
		if (fstat(pipe_fd.fds, &st) < 0)
			initctl_control_close();
		if (stat(INIT_FIFO, &st2) < 0 || st.st_dev != st2.st_dev
			|| st.st_ino != st2.st_ino)
		{
			initctl_control_close();
		}
	}

	/* If the pipe isn't open, try to open it. */
	if (pipe_fd.fds < 3)
	{
		/* it the file descriptor has to be over 2 to be valid */
		if ((pipe_fd.fds = open(INIT_FIFO, O_RDWR | O_NONBLOCK)) < 3)
			return (FALSE);

		D_("Opened on fd %i\n", pipe_fd.fds);
		fstat(pipe_fd.fds, &st);
		if (!S_ISFIFO(st.st_mode))
		{
			/* /dev/initctl is there, but we can't open it */
			F_("%s is not a fifo\n", INIT_FIFO);
			if (pipe_fd.fds >= 0)
			{
				close(pipe_fd.fds);
				pipe_fd.fds = -1;
			}
			return (FALSE);
		}

		/* ok, finally add hook */
		initng_plugin_hook_register(&g.FDWATCHERS, 70, &pipe_fd);
	}
	return (TRUE);
}

/* To be called when there is input on the control bus */
void parse_control_input(f_module_h * from_module, e_fdw what)
{
	int n;
	struct init_request request;

	if (from_module != &pipe_fd)
		return;

	/* Read data from /dev/initctl */
	n = read(pipe_fd.fds, &request, sizeof(request));

	/* Check if request is ok : */
	if (n == 0)
	{
		F_("read 0 bytes, this should never happen!\n");
		return;
	}
	if (n <= 0)
	{
		if (errno == EINTR)
			return;
		F_("Error reading request\n");
		return;
	}
	if (request.magic != INIT_MAGIC || n != sizeof(request))
	{
		F_("got bogus init request\n");
		return;
	}

	/*
	 * Check that the request command is a valid one.
	 */
	if (request.cmd != INIT_CMD_RUNLVL)
	{
		D_("got unimplemented init request - %d (%c),%d (%c).\n",
		   request.runlevel, request.runlevel, request.cmd, request.cmd);
		return;
	}

	/* TODO, handle these:
	   #define INIT_CMD_START              0
	   #define INIT_CMD_RUNLVL        1
	   #define INIT_CMD_POWERFAIL     2
	   #define INIT_CMD_POWERFAILNOW  3
	   #define INIT_CMD_POWEROK       4
	 */

	/* Request is OK, handle it: */
	D_("init data is : - %d (%c),%d (%c).\n", request.runlevel,
	   request.runlevel, request.cmd, request.cmd);

	switch (request.runlevel)
	{
			/* halting */
		case '0':
			D_("Halting.\n");
			g.when_out = THEN_POWEROFF;
			initng_handler_stop_all();
			return;

			/* reboot */
		case '6':
			D_("Rebooting.\n");
			g.when_out = THEN_REBOOT;
			initng_handler_stop_all();
			return;

			/* restart init */
		case 'U':
		case 'u':
			D_("init U, sent reloading initng\n");
			initng_reload();
			return;

#ifdef SERVICE_CACHE
			/* reload /etc/inittab */
		case 'Q':
		case 'q':
			D_("init Q, freeing complete service cache\n");
			initng_service_cache_free_all();
			return;
#endif

			/* go singleuser */
		case 'S':
		case 's':
			W_("init S, going singleuser\n");
			g.when_out = THEN_RESTART;

			/* set next runlevel to single (That will be loaded when no service is left in current one) */
			initng_main_set_runlevel("single");
			initng_handler_stop_all();
			return;
		default:
			D_("Starting runlevel%c\n", request.runlevel);
			{
				char tmp[20];

				sprintf(tmp, "runlevel%c", request.runlevel);
				if (!initng_handler_start_new_service_named(tmp))
					F_(" service \"%s\" could not be executed.\n", tmp);
			}
			return;
	}
}

static void makeutmp(int runlevel)
{
	D_("Making utmp file for runlevel %d\n", runlevel);
	struct utmp utmp;
	time_t t;

	/*
	 * this is created by bootmisc, if this isn't there we can't set runlevel.
	 */
	if (access(UTMP_FILE, F_OK) < 0)
	{
		F_("/var/run/utmp does not exist, this should be created by bootmisc.i\n");
		return;
	}
	/*
	   TODO, is this a good idea or a bad idea?
	   utmpname("/var/run/utmp");
	 */

	setutent();
	memset(&utmp, 0, sizeof(utmp));
	utmp.ut_type = RUN_LVL;
	utmp.ut_pid = ('#' << 8) + runlevel + '0';
	time(&t);
	utmp.ut_time = (int) t;
	if (pututline(&utmp) == NULL)
	{
		F_("pututline failed\n");
		endutent();
		return;
	}
	endutent();
	return;
}

static void initng_reload(void)
{
	s_command *reload = initng_command_find_by_command_id('c');

	if (reload && reload->u.void_command_call)
	{
		(*reload->u.void_command_call) (NULL);
	}
}



/* try open FIFO, every started service */
static void hup_request(int signal)
{
	/* Look for the right signal */
	if (signal != SIGHUP)
		return;

	if (!initctl_control_open())
	{
		F_("Warning, failed to open /dev/initctl\n");
	}
}

static int is_system_up(s_event * event)
{
	h_sys_state * state;

	assert(event->event_type == &EVENT_SYSTEM_CHANGE);
	assert(event->data);

	state = event->data;

	if (*state == STATE_UP && (!utmp_stored))
	{
		makeutmp(3);
	}

	return (TRUE);
}


int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	if (g.i_am != I_AM_INIT)
	{
		initng_unload_module_named("initctl");
		return (TRUE);						/* this is not a failure */
	}

	utmp_stored = FALSE;

	initctl_control_open();

	if ((!initng_plugin_hook_register(&g.SIGNAL, 50, &hup_request))
		|| (!initng_event_hook_register(&EVENT_SYSTEM_CHANGE, &is_system_up)))
	{
		F_("Fail add hook!\n");
		return (FALSE);
	}
	return (TRUE);
}


void module_unload(void)
{
	D_("module_unload();\n");

	/* then this module was never loaded */
	if (g.i_am != I_AM_INIT)
		return;								/* this is not a failure */


	initctl_control_close();
	/* remove all hooks */
	initng_plugin_hook_unregister(&g.FDWATCHERS, &pipe_fd);
	initng_event_hook_unregister(&EVENT_SYSTEM_CHANGE, &is_system_up);
	initng_plugin_hook_unregister(&g.SIGNAL, &hup_request);
}
