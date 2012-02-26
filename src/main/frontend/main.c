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

#include <unistd.h>
#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <stdlib.h>		/* free() exit() */
#include <termios.h>
#include <stdio.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/klog.h>
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <sys/ioctl.h>		/* ioctl() */
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/mount.h>

#include <initng.h>
#include <initng-paths.h>
#include "options.h"
#define TIMEOUT 60000

int main(int argc, char *argv[], char *env[])
{
	struct timeval last;	/* save the time here */
	int retval;

#ifdef DEBUG
	int loop_counter = 0;	/* counts how many times the main_loop
				 * has run */
#endif
	S_;

	/* get the time */
	gettimeofday(&last, NULL);

	/* Initialize global variables */
	initng_config_global_new(argc, argv, env);

	/* Parse options given on argv. */
	options_parse_args(argv);

	/* when last service stopped, offer a sulogin */
	g.when_out = THEN_SULOGIN;
	if (!g.runlevel)
		initng_main_set_runlevel(RUNLEVEL_DEFAULT);

	D_("MAIN_LOAD_MODULES\n");
	/* Load modules, if fails - launch sulogin and then try again */
	if (!initng_module_load_all(INITNG_MODULE_DIR)) {
		/* if we can't load modules, revert to single-user login */
		initng_main_su_login();
	}

	if (g.hot_reload) {
		/* Mark service as up */
		retval = initng_module_callers_active_db_reload();
		if (retval != TRUE) {
			if (retval == FALSE)
				F_("No module handled hot_reload!\n");
			else
				F_("Hot reload failed!\n");

			/* if the hot reload failed, we're probably in big
			 * trouble */
			initng_main_su_login();
		}

		/* Hopefully no-one will try a hot reload when the system
		 * isn't up... */
		initng_main_set_sys_state(STATE_UP);
	}

	/* set title of initng, can be watched in ps -ax */
	initng_toolbox_set_proc_title("initng [%s]", g.runlevel);

	/* Configure signal handlers */
	initng_signal_enable();

	/* make sure this is not a hot reload */
	if (!g.hot_reload) {
		/* change system state */
		initng_main_set_sys_state(STATE_STARTING);

		/* first start all services, prompted at boot with
		 * +daemon/test */
		initng_main_start_extra_services();

		/* try load the default service, if it fails - launch sulogin
		 * and try again */
		if (!initng_handler_start_new_service_named(g.runlevel)) {
			F_("Failed to load runlevel (%s)!\n", g.runlevel);
			initng_main_su_login();
		}
	}

	D_("MAIN_GOING_MAIN_LOOP\n");
	/* %%%%%%%%%%%%%%%   MAIN LOOP   %%%%%%%%%%%%%%% */
	for (;;) {
		D_("MAIN_LOOP: %i\n", loop_counter++);
		int interrupt = FALSE;

		/* Update current time, save this in global so we don't need
		 * to call time() that often. */
		gettimeofday(&g.now, NULL);

		/*
		 * If it has gone more then 1h since last
		 * main loop, or if clock has gone backwards,
		 * reset it.
		 */
		if ((g.now.tv_sec - last.tv_sec) >= 3600 ||
		    (g.now.tv_sec - last.tv_sec) < 0) {
			D_(" Clock skew, time have changed over one hour, in "
			   "one mainloop, compensating %i seconds.\n",
			   MS_DIFF(g.now, last));
			initng_active_db_compensate_time(g.now.tv_sec -
							 last.tv_sec);
			initng_module_callers_compensate_time(g.now.tv_sec -
							      last.tv_sec);
			last.tv_sec += (g.now.tv_sec - last.tv_sec);
		}

		/* put last time, to current time last = g.now; */
		memcpy(&last, &g.now, sizeof(struct timeval));

		/* if a sleep is set */
		g.sleep_seconds = 0;

		/* run state alarm if timeout */
		/*
		 * Run all alarm state callers.
		 * If there is more later alarms, this will set new ones.
		 */
		if (g.next_alarm && g.next_alarm <= g.now.tv_sec)
			initng_handler_run_alarm();

		/* handle signals */
		initng_signal_dispatch();

		/* If there is modules to unload, handle this */
		if (g.modules_to_unload == TRUE) {
			g.modules_to_unload = FALSE;
			D_("There is modules to unload!\n");
			initng_module_unload_marked();
		}

		/*
		 * After here i put things that should be done every main
		 * loop, g.interrupt or not ...
		 */

		/*
		 * Trigger MAIN_EVENT event.
		 * OBS! This part in mainloop is expensive.
		 */
		{
			s_event event;
			event.event_type = &EVENT_MAIN;
			initng_event_send(&event);
		}

		/*
		 * Check if there are any running processes left, otherwise
		 * quit, when_out try to do something when this happens.
		 *
		 * This check is also expensive.
		 */
		if (initng_main_ready_to_quit() == TRUE) {
			initng_main_set_sys_state(STATE_ASE);
			P_(" *** Last service has quit. ***\n");
			initng_main_when_out();
			continue;
		}

		/*
		 * Run this to clean the active_db out from down services.
		 */
		initng_active_db_clean_down();

		/* LAST: check for service state changes in a special
		 * function */
		interrupt = initng_interrupt();

		/* figure out how long we can sleep */
		{
			/* set it to default */
			int closest_timeout = TIMEOUT;

			/* if sleepseconds are set */
			if (g.sleep_seconds &&
			    g.sleep_seconds < closest_timeout)
				closest_timeout = g.sleep_seconds;

			/* Check how many seconds there are to the state
			 * alarm */
			if (g.next_alarm && g.next_alarm > g.now.tv_sec) {
				int time_to_next = g.next_alarm - g.now.tv_sec;

				D_("g.next_alarm is set!, will trigger in %i "
				   "sec.\n", time_to_next);
				if (time_to_next < closest_timeout)
					closest_timeout = time_to_next;
			}

			/* if we got some seconds */
			if (closest_timeout > 0 && interrupt == FALSE) {
				/* do a poll == the same as sleep but also
				 * watch fds */
				D_("Will sleep for %i seconds.\n",
				   closest_timeout);
				initng_io_module_poll(closest_timeout);
			}
		}
	}			/* End main loop */
}
