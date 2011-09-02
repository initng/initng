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

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <unistd.h>		/* execv() usleep() pause() chown() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdlib.h>		/* free() exit() */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};

s_entry EXEC = {
	.name = "exec",
	.description = "Contains the path and arguments to a file to exec.",
	.type = VARIABLE_STRING,
	.ot = NULL,
};

s_entry EXECS = {
	.name = "exec_path",
	.description = "The path for one or more executables.",
	.type = VARIABLE_STRINGS,
	.ot = NULL,
};

s_entry EXEC_ARGS = {
	.name = "exec_args",
	.description = "The arguments for the executable.",
	.type = VARIABLE_STRING,
	.ot = NULL,
};

/*
 * Searches for exec in PATH.
 *
 * Completely rewritten function.
 * Contains old Code from SaTaN0rX and DEac-.
 * @author TheLich
 * @param exec filename, which searched.
 * @return executable file with absolute path.\nIf exec was absolute, this will be a pointer to exec.
 * On failure, it will return NULL.
 */

static char *expand_exec(char *exec)
{
	char *filename = NULL;
	size_t exec_len = 0;
	size_t i, len = 0;
	char *PATH = NULL;
	char **path_argv = NULL;
	struct stat test;

	if (!exec)
		return NULL;

	/* 11/20/2005 SaTaN0rX: preliminary support for searching the PATH
	 * only search the PATH if this is NOT already an absolute path
	 */

	/* if path provided, just return */
	if (exec[0] == '/')
		return NULL;

	/* get exec string length to later use */
	exec_len = strlen(exec);

	D_("initng_s_launch: %s is not an absolute path, searching $PATH\n",
	   exec);

	/* get the env-path variable */
	PATH = getenv("PATH");

	/* Make sure we got a path */
	if (!PATH) {
		D_("No $PATH found, using default path\n");
		PATH =
		    initng_toolbox_strdup
		    ("/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin");
	} else {
		/* PATH will later be changed, so we have to use a duplicate */
		PATH = initng_toolbox_strdup(PATH);
	}

	D_("PATH determined to be %s\n", PATH);

	/* split path by ':' char */
	path_argv = initng_string_split_delim(PATH, ":", NULL);

	/* walk the list of entries */
	for (i = 0; path_argv[i]; i++) {
		/* get entry length */
		len = strlen(path_argv[i]);

		/* what does this do? */
		filename = (char *)initng_toolbox_calloc(exec_len + len + 2, 1);
		strcpy(filename, path_argv[i]);
		if (filename[len - 1] != '/')
			strcat(filename, "/");

		strcat(filename, exec);

		/* check so that file exits, if exists leave this loop */
		if ((stat(filename, &test) != -1) &&
		    (test.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR | S_ISVTX |
				     S_ISGID | S_ISUID | S_IFREG)))
			break;

		/* cleanup */
		free(filename);
		filename = NULL;
	}

	/* free */
	free(PATH);
	initng_string_split_delim_free(path_argv);
	PATH = NULL;
	path_argv = NULL;

	/* return the filename */
	return filename;
}

static int simple_exec_fork(process_h * process_to_exec, active_db_h * s,
			    size_t argc, char **argv)
{
	/* This is the real service kicker */
	pid_t pid_fork;		/* pid got from fork() */

	pid_fork = initng_fork(s, process_to_exec);
	if (pid_fork == 0) {
		/* run g.AFTER_FORK from other modules */
		initng_fork_aforkhooks(s, process_to_exec);

#ifdef DEBUG
		D_("FROM_FORK simple_exec(%i,%s, ...);\n", argc, argv[0]);
		/*D_argv("simple_exec: ", argv); */
#endif

		/* FINALLY EXECUTE *//* execve replaces the running process */
		execve(argv[0], argv, initng_env_new(s));

		/* Will never get here if execve succeeded */
		F_("ERROR!\n");
		F_("Can't execute source %s!\n", argv[0]);

		_exit(1);
	}

	/* save pid of fork */
	D_("FROM_FORK Forkstarted pid %i.\n", pid_fork);

	if (pid_fork > 0)
		return TRUE;

	process_to_exec->pid = 0;
	return FALSE;
	/* if to test want to lock this up until fork is done ...
	 * waitpid(pid_fork,0,0); */
}

static int simple_exec_try(const char *exec, active_db_h * service,
			   process_h * process)
{
	char *exec_args = NULL;
	char **argv = NULL;
	size_t argc = 0;
	int ret;

	D_("exec: %s, service: %s, process: %s\n", exec, service->name,
	   process->pt->name);

	/* exec_args should be parsed at the moment, too */
	exec_args = (char *)get_string_var(&EXEC_ARGS, process->pt->name,
					   service);
	if (exec_args) {
		initng_string_fix_escapes(exec_args);

		/* split the string, with entries to an array of strings */
		argv =
		    initng_string_split_delim(exec_args + 1, WHITESPACE, &argc);

		/* make sure it succeeded */
		if (!argv || !argv[0]) {
			if (argv)
				initng_string_split_delim_free(argv);

			F_("initng_string_split_delim exec_args returns NULL.\n");
			return FALSE;
		}
	} else {
		/* we need a empty argv anyway */
		argv = (char **)initng_toolbox_calloc(2, sizeof(char *));
		argv[1] = NULL;
		argc = 0;
	}

	argv[0] = (char *)exec;

	ret = simple_exec_fork(process, service, argc, argv);

	if (argv)
		initng_string_split_delim_free(argv);

	return ret;
}

static int simple_exec(active_db_h * service, process_h * process)
{
	const char *exec = NULL;
	struct stat stat_struct;
	s_data *itt = NULL;

	D_("service: %s, process: %s\n", service->name, process->pt->name);

	while ((exec = get_next_string_var(&EXECS, process->pt->name, service,
					   &itt))) {
		int res = FALSE;

		/* check if the file exist */
		if (stat(exec, &stat_struct) != 0) {
			D_(" note, %s exec_fixed does not exist. \n", exec);
			continue;
		}

		/* Try to execute that one */
		res = simple_exec_try(exec, service, process);

		/* Return true if successfully */
		if (res == TRUE)
			return TRUE;
	}

	return FALSE;
}

static int simple_run(active_db_h * service, process_h * process)
{
	char *exec = NULL;
	char **argv = NULL;
	size_t argc = 0;
	int result = FALSE;
	char *argv0 = NULL;

	D_("service: %s process: %s.\n", service->name, process->pt->name);

	exec = (char *)get_string_var(&EXEC, process->pt->name, service);
	if (!exec)
		return FALSE;

	initng_string_fix_escapes(exec);

	/* argv-entries are pointer to exec_t[x] */
	argv = initng_string_split_delim(exec, WHITESPACE, &argc);

	/* make sure we got something from the split */
	if (!argv || !argv[0]) {
		if (argv)
			initng_string_split_delim_free(argv);

		D_("initng_string_split_delim on exec returns NULL.\n");
		return FALSE;
	}

	/* if it not contains a full path */
	if (argv[0][0] != '/') {
		argv0 = expand_exec(argv[0]);
		if (!argv0) {
			F_("SERVICE: %s %s -- %s was not found in search "
			   "path.\n", service->name, process->pt->name,
			   argv[0]);
			initng_string_split_delim_free(argv);
			argv = NULL;

			return FALSE;
		}

		free(argv[0]);
		argv[0] = argv0;	/* Check this before freeing! */
	}

	/* try to execute, remember the result */
	result = simple_exec_fork(process, service, argc, argv);

	/* clean up */

	/* First free the fixed argv0 if its not a plain link to argv[0] */
	if (argv0 && argv0 != argv[0])
		free(argv0);

	argv0 = NULL;

	// Later free the big argv array
	initng_string_split_delim_free(argv);
	argv = NULL;

	/* return result */
	if (result == FAIL)
		return FALSE;

	return result;
}

static void initng_s_launch(s_event * event)
{
	s_event_launch_data *data;

	assert(event->event_type == &EVENT_LAUNCH);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->service->name);
	assert(data->process);
	assert(data->exec_name);

	D_("service: %s, process: %s\n", data->service->name,
	   data->process->pt->name);

	if (is_var(&EXECS, data->exec_name, data->service)) {
		if (simple_exec(data->service, data->process)) {
			event->status = HANDLED;
			return;
		}
	}

	if (is_var(&EXEC, data->exec_name, data->service)) {
		if (simple_run(data->service, data->process))
			event->status = HANDLED;
	}
}

int module_init(void)
{
	initng_event_hook_register(&EVENT_LAUNCH, &initng_s_launch);
	initng_service_data_type_register(&EXEC);
	initng_service_data_type_register(&EXECS);
	initng_service_data_type_register(&EXEC_ARGS);

	return TRUE;
}

void module_unload(void)
{
	initng_service_data_type_unregister(&EXEC);
	initng_service_data_type_unregister(&EXECS);
	initng_service_data_type_unregister(&EXEC_ARGS);
	initng_event_hook_unregister(&EVENT_LAUNCH, &initng_s_launch);
}
