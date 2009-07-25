/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2005 neuron <aagaande@gmail.com>
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
#include <stdlib.h>		/* free() exit() */
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
};

s_entry STDOUT = {
	.name = "stdout",
	.description = "Open a file with this path, and direct service "
	    "output here.",
	.type = STRING,
	.ot = NULL,
};

s_entry STDERR = {
	.name = "stderr",
	.description = "Open a file with this path, and direct service error "
	    "output here.",
	.type = STRING,
	.ot = NULL,
};

s_entry STDALL = {
	.name = "stdall",
	.description = "Open a file with this path, and direct service all "
	    "output here.",
	.type = STRING,
	.ot = NULL,
};

s_entry STDIN = {
	.name = "stdin",
	.description = "Open a file with this path, and direct service input "
	    "here.",
	.type = STRING,
	.ot = NULL,
};

static void setup_output(s_event * event)
{
	s_event_after_fork_data *data;

	/* string containing the filename of output */
	const char *s_stdout = NULL;
	const char *s_stderr = NULL;
	const char *s_stdall = NULL;
	const char *s_stdin = NULL;

	/* file descriptors */
	int fd_stdout = -1;
	int fd_stderr = -1;
	int fd_stdall = -1;
	int fd_stdin = -1;

	assert(event->event_type == &EVENT_AFTER_FORK);
	assert(event->data);

	data = event->data;

	/* assert some */
	assert(data->service);
	assert(data->service->name);
	assert(data->process);

	S_;

	/* get strings if present */
	s_stdout = get_string(&STDOUT, data->service);
	s_stderr = get_string(&STDERR, data->service);
	s_stdall = get_string(&STDALL, data->service);
	s_stdin = get_string(&STDIN, data->service);

	if (!(s_stdout || s_stderr || s_stdall || s_stdin)) {
		D_("This plugin won't do anything, because no opt set!\n");
		return;
	}

	/* if stdout points to same as stderr, set s_stdall */
	if (s_stdout && s_stderr && strcmp(s_stdout, s_stderr) == 0) {
		s_stdall = s_stdout;
		s_stdout = NULL;
		s_stderr = NULL;
	}

	/* if stdall string is set */
	if (s_stdall) {
		/* output all to this */
		fd_stdall = open(s_stdall,
				 O_WRONLY | O_NOCTTY | O_CREAT | O_APPEND,
				 0644);
	} else {
		/* else set them to different files */
		if (s_stdout)
			fd_stdout = open(s_stdout,
					 O_WRONLY | O_NOCTTY | O_CREAT |
					 O_APPEND, 0644);
		if (s_stderr)
			fd_stderr = open(s_stderr,
					 O_WRONLY | O_NOCTTY | O_CREAT |
					 O_APPEND, 0644);
	}

	if (s_stdin)
		fd_stdin = open(s_stdin, O_RDONLY | O_NOCTTY, 0644);

	/* print fail messages, if the files did not open */
	if (s_stdall && fd_stdall < 2)
		F_("StdALL: %s, fd %i\n", s_stdall, fd_stdall);
	if (s_stdout && fd_stdout < 2)
		F_("StdOUT: %s, fd %i\n", s_stdout, fd_stdout);
	if (s_stderr && fd_stderr < 2)
		F_("StdERR: %s, fd %i\n", s_stderr, fd_stderr);
	if (s_stdin && fd_stdin < 2)
		F_("StdIN: %s, fd %i\n", s_stdin, fd_stdin);

#ifdef DEBUG
	if (s_stdall)
		D_("StdALL: %s, fd %i\n", s_stdall, fd_stdall);
	if (s_stdout)
		D_("StdOUT: %s, fd %i\n", s_stdout, fd_stdout);
	if (s_stderr)
		D_("StdERR: %s, fd %i\n", s_stderr, fd_stderr);
	if (s_stdin)
		D_("StdIN:  %s, fd %i\n", s_stdin, fd_stdin);
#endif

	/* if fd succeeded to open */
	if (fd_stdall > 2) {
		/* dup both to stdall */
		dup2(fd_stdall, 1);
		dup2(fd_stdall, 2);

		initng_fd_set_cloexec(fd_stdall);
	} else {
		/* else dup them to diffrent fds */
		if (fd_stdout > 2)
			dup2(fd_stdout, 1);
		if (fd_stderr > 2)
			dup2(fd_stderr, 2);

		initng_fd_set_cloexec(fd_stdout);
		initng_fd_set_cloexec(fd_stderr);
	}

	if (fd_stdin > 2) {
		dup2(fd_stdin, 0);
		initng_fd_set_cloexec(fd_stdin);
	}
}

int module_init(void)
{
	initng_service_data_type_register(&STDOUT);
	initng_service_data_type_register(&STDERR);
	initng_service_data_type_register(&STDALL);
	initng_service_data_type_register(&STDIN);

	initng_event_hook_register(&EVENT_AFTER_FORK, &setup_output);
	return TRUE;
}

void module_unload(void)
{
	initng_service_data_type_unregister(&STDOUT);
	initng_service_data_type_unregister(&STDERR);
	initng_service_data_type_unregister(&STDALL);
	initng_service_data_type_unregister(&STDIN);

	initng_event_hook_unregister(&EVENT_AFTER_FORK, &setup_output);
}
