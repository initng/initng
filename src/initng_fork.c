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

#include "initng.h"

#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <unistd.h>							/* execv() pipe() usleep() pause() chown() pid_t */
#include <sys/wait.h>						/* waitpid() sa */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdlib.h>							/* free() exit() */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>


#include "initng_active_db.h"
#include "initng_process_db.h"
#include "initng_signal.h"
#include "initng_plugin_callers.h"
#include "initng_execute.h"
#include "initng_toolbox.h"

#include "initng_fork.h"



pid_t initng_fork(active_db_h * service, process_h * process)
{
	/* This is the real service kicker */
	pid_t pid_fork;				/* pid got from fork() */
	int try_count = 0;			/* Count tryings */
	s_call *current = NULL;

	assert(service);
	assert(process);

	/* open pipe */
	if (pipe(process->out_pipe) != 0)
	{
		F_("Failed adding pipe ! %s\n", strerror(errno));
		return (-1);
	}

	/* alloc buffer */
	if (process->buffer)
	{
		free(process->buffer);
		process->buffer = NULL;
		process->buffer_allocated = 0;
	}


	/* Try to fork 30 times */
	while ((pid_fork = fork()) == -1)
	{
		if (try_count > 30)
		{									/* Already tried 30 times = no more try */
			F_("GIVING UP TRY TO FORK, FATAL for service.\n");
			return (pid_fork);
		}
		F_("FAILED TO FORK! try no# %i, this can be FATAL!\n", try_count);
		usleep(++try_count * 2000);			/* Sleep a while before trying again */
	}

	/* if this is child */
	if (pid_fork == 0)
	{
		/* from now, don't trap signals */
		initng_signal_disable();

#ifdef DEBUG
		/* unverbose by default for now */
		/* g.verbose = 0; */
#endif


		if (g.i_am != I_AM_UTILITY)
		{
			/* TODO, comment this */
			if (g.i_am == I_AM_INIT)
			{
				ioctl(0, TIOCNOTTY, 0);
				setsid();					/* Run a program in a new session ??? */
			}

			/*
			 * set up file descriptors, for local fork,
			 * a fork in initng, should now receive any input, but stdout & stderr, should be sent
			 * to process->out_pipe[], that is set up by pipe() #man 2 pipe
			 * [0] for reading, and [1] for writing, as the pipe is for sending output
			 * FROM the fork, to initng for handle, the input part should be closed here,
			 * the other are mapped to STDOUT and STDERR.
			 */

			/* close stdin/stdout/stderr */
			/*close(STDIN_FILENO); */
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

			/* Duplicate stdout and stderr to the stdout[1] */
			dup2(process->out_pipe[1], STDOUT_FILENO);
			dup2(process->out_pipe[1], STDERR_FILENO);

			/* set stdin, stdout, and stderr, that is should not be closed, if this child do execve() */
			fcntl(STDIN_FILENO, F_SETFD, 0);
			fcntl(STDOUT_FILENO, F_SETFD, 0);
			fcntl(STDERR_FILENO, F_SETFD, 0);

			/* Close the sides of the pipes we don't need, as we're in fork we won't need this part. */
			if (process->out_pipe[0] > 0)
				close(process->out_pipe[0]);
			process->out_pipe[0] = -1;

		}

		/* There might be plug-ins that will work here */
		while_list(current, &g.A_FORK)
		{
			if (((*current->c.af_launcher) (service, process)) == FALSE)
			{
				F_("Some plugin did fail (from:%s), in after fork launch.\n",
				   current->from_file);
				_exit(1);
			}
		}

		/* close all open fds, except STDIN, STDOUT, STDERR (0, 1, 2) */
		{
			int i;

			for (i = 3; i <= 1013; i++)
				close(i);
		}

		/* TODO, what does this do? */
		if (g.i_am == I_AM_INIT)
			tcsetpgrp(0, getpgrp());		/* run this in foreground on fd 0 */

		/* do a minimum sleep, to let the mother process 
		   to register child, and notice death */
		usleep(ALL_USLEEP);

	}
	else
	{

		/* close the receiving end on pipe, on parent */
		if (process->out_pipe[1] > 0)
			close(process->out_pipe[1]);
		process->out_pipe[1] = -1;
		if (pid_fork > 0)
		{
			process->pid = pid_fork;
		}
	}

	return (pid_fork);
}
