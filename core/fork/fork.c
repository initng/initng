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

#include <unistd.h>		/* execv() pipe() pause() chown() pid_t */
#include <time.h>		/* time() nanosleep() */
#include <fcntl.h>		/* fcntl() */

#include <sys/types.h>
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <sys/socket.h>		/* socketpair() */
#include <stdlib.h>		/* free() exit() */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>

#include <initng.h>

inline static void pipes_create(process_h *process)
{
	pipe_h *current_pipe = NULL;

	while_pipes(current_pipe, process) {
		if (current_pipe->dir == IN_AND_OUT_PIPE) {
			/* create an two directional pipe with socketpair */
			if (socketpair(AF_UNIX, SOCK_STREAM, 0,
				       current_pipe->pipe) < 0) {
				F_("Fail call socketpair: \"%s\"\n",
				   strerror(errno));
			}
		} else {
			/* create an one directional pipe with pipe */
			if (pipe(current_pipe->pipe) != 0) {
				F_("Failed adding pipe ! %s\n",
				   strerror(errno));
			}
		}
	}
}

/**
 * Walk all pipes and close all remote sides of pipes
 */
inline static void pipes_close_remote_side(process_h *process)
{
	pipe_h *pipe = NULL;

	while_pipes(pipe, process) {
		switch (pipe->dir) {
		case OUT_PIPE:
		case BUFFERED_OUT_PIPE:
			if (pipe->pipe[1] > 0)
				close(pipe->pipe[1]);
			pipe->pipe[1] = -1;
			break;

		case IN_PIPE:
		case IN_AND_OUT_PIPE:
			/* close the OUTPUT end */
			/* in an unidirectional pipe, pipe[0] is fork,
			 * and pipe[1] is parent */
			if (pipe->pipe[0] > 0)
				close(pipe->pipe[0]);
			pipe->pipe[0] = -1;
			break;
		}
	}
}


/**
 * set up file descriptors, for local fork, a fork in initng, should now receive
 * any input, but stdout & stderr, should be sent to process->out_pipe[], that
 * is set up by pipe() #man 2 pipe [0] for reading, and [1] for writing, as the
 * pipe is for sending output FROM the fork, to initng for handle, the input
 * part should be closed here, the other are mapped to STDOUT and STDERR.
 */
inline static void pipes_setup_local_side(process_h *process)
{
	pipe_h *pipe = NULL;

	/* walk thru all the added pipes */
	while_pipes(pipe, process) {
		int i;

		/* for every target */
		for (i = 0; pipe->targets[i] > 0 && i < MAX_TARGETS; i++) {
			/* close any conflicting one */
			close(pipe->targets[i]);

			switch(pipe->dir) {
			case OUT_PIPE:
			case BUFFERED_OUT_PIPE:
				/* duplicate the new target right */
				dup2(pipe->pipe[1], pipe->targets[i]);
				break;
			case IN_PIPE:
			case IN_AND_OUT_PIPE:
				/* duplicate the input pipe instead */
				/* in a unidirectional socket, there is pipe[0]
				   that is used in the child */
				dup2(pipe->pipe[0], pipe->targets[i]);
				break;
			}


			/* IMPORTANT: Tell the os not to close the new target on
			   execve */
			fcntl(pipe->targets[i], F_SETFD, 0);
		}
	}
}



pid_t initng_fork(active_db_h * service, process_h * process)
{
	/* This is the real service kicker */
	pid_t pid_fork;		/* pid got from fork() */
	int try_count = 0;	/* Count tryings */

	assert(service);
	assert(process);

	/* Create all pipes */
	pipes_create(process);

	/* Try to fork 30 times */
	while ((pid_fork = fork()) == -1) {
		if (try_count > 30) {	/* Already tried 30 times = no more try */
			F_("GIVING UP TRY TO FORK, FATAL for service.\n");
			return pid_fork;
		}
		F_("FAILED TO FORK! try no# %i, this can be FATAL!\n",
		   try_count);

		/* sleep a while before trying again */
		struct timespec nap;
		nap.tv_nsec = 2000 * ++try_count;
		nanosleep(&nap, NULL);
	}

	/* if this is child */
	if (pid_fork == 0) {
		/* from now, don't trap signals */
		initng_signal_disable();

		/* TODO, comment this */
#ifndef __HAIKU__
		ioctl(0, TIOCNOTTY, 0);
#endif
		setsid();	/* Run a program in a new
				 * session ??? */

		pipes_setup_local_side(process);

		/* TODO, what does this do? */
		/* run this in foreground on fd 0 */
		tcsetpgrp(0, getpgrp());

		/* do a minimum sleep, to let the mother process
		   to register child, and notice death */
		struct timespec nap = { 0, ALL_NANOSLEEP };
		nanosleep(&nap, NULL);
	} else {
		pipes_close_remote_side(process);

		/* set process->pid if lucky */
		if (pid_fork > 0)
			process->pid = pid_fork;
	}

	return pid_fork;
}
