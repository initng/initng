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

#include <sys/wait.h>				/* waitpid() sa */
#include <stdlib.h>				/* free() exit() */
#include <stdio.h>				/* printf() */
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "local.h"


/*
 * If we got an child that died, this handler is called.
 */
void initng_signal_handle_sigchild(void)
{
	int status;					/* data got from waitpid, never used */
	pid_t killed;				/* pid of killed app */

	for (;;)
	{
		/* slaying zombies */
		do
		{
			killed = waitpid(-1, &status, WNOHANG);
		}
		while ((killed < 0) && (errno == EINTR));

		/* Nothing killed */
		if (killed == 0)
		{
			return;
		}

		/* unknown child */
		if ((killed < 0) && (errno == ECHILD))
		{
			return;
		}

		/* Error */
		if (killed < 0)
		{
			W_("Error, waitpid returned pid %d (%s)\n", killed,
			   strerror(errno));
			return;
		}

		D_("sigchild(): PID %i KILLED WITH:\n"
		   "WEXITSTATUS %i\n"
		   "WIFEXITED %i\n"
		   "WIFSIGNALED %i\n"
		   "WTERMSIG %i\n"
		   "WCOREDUMP %i\n"
		   "WIFSTOPPED %i\n"
		   "WSTOPSIG %i\n"
		   "\n",
		   killed,
		   WEXITSTATUS(status),
		   WIFEXITED(status),
		   WIFSIGNALED(status),
		   WTERMSIG(status),
		   WCOREDUMP(status), WIFSTOPPED(status), WSTOPSIG(status));

		/*if(WTERMSIG(status))
		   {
		   printf("Service segfaulted!\n");
		   } */

		/*
		 * call handle_killed_by_pid(), and will walk the active_db
		 * setting the a_status of the service touched
		 */
		initng_kill_handler_killed_by_pid(killed, status);
	}
}

/* called by signal SIGSEGV */
void sigsegv(int sig)
{
	(void) sig;
	printf("SEGFAULTED!\n");
	initng_main_segfault();
}
