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

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <sys/un.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdlib.h>		/* free() exit() */
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <sys/mount.h>
#include <termios.h>
#include <stdio.h>
#include <sys/klog.h>
#include <errno.h>
#ifdef SELINUX
#include <selinux/selinux.h>
#include <selinux/get_context_list.h>
#endif

static int local_sulogin_count = 0;

#define TRY_TIMES 2
/*
 *  Launch sulogin and wait it for finish
 */
void initng_main_su_login(void)
{
	pid_t sulogin_pid;
	int status;

#ifdef SELINUX
	if (is_selinux_enabled > 0) {
		security_context_t *contextlist = NULL;

		if (get_ordered_context_list("root", 0, &contextlist) > 0) {
			if (setexeccon(contextlist[0]) != 0)
				fprintf(stderr, "setexeccon failed\n");
			freeconary(contextlist);
		}
	}
#endif
	/* sulogin nicely 2 times */
	if (local_sulogin_count <= TRY_TIMES) {
		printf("This is a sulogin offer,\n"
		       "you will be able to login for %i times (now %i),\n"
		       "and on return initng will try continue where it was,\n"
		       "if the times go out, initng will launch\n"
		       "/sbin/initng-segfault on next su_login request.\n\n",
		       TRY_TIMES, local_sulogin_count);
		sulogin_pid = fork();

		if (sulogin_pid == 0) {
			const char *sulogin_argv[] = { "/sbin/sulogin", NULL };
			const char *sulogin_env[] = { NULL };

			/* launch sulogin */
			execve(sulogin_argv[0], (char **)sulogin_argv,
			       (char **)sulogin_env);

			printf("Unable to execute /sbin/sulogin!\n");
			_exit(1);
		}

		if (sulogin_pid > 0) {
			do {
				sulogin_pid = waitpid(sulogin_pid, &status,
						      WUNTRACED);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));

			/* increase the sulogin_count */
			local_sulogin_count++;
			return;
		}
	}

	/* else run segfault script */
	initng_main_segfault();

	/* newer get here */
	_exit(1);
}
