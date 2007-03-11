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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <initng-paths.h>

#define WRAPPER_PATH INITNG_PLUGIN_DIR "/wrappers/"

extern char **environ;

char *wrapper = "default";

/* These commands will be forwarded to /sbin/ngc if issued */
const char *ngc_args[] = { "start", "stop", "restart", "zap", "status", NULL
};

/* this lists the commands the service can be executed with directly */
static void print_usage(void)
{
	int i;

	printf("Usage: /etc/init/service");
	for (i = 0; ngc_args[i]; i++)
		printf(" <%s>", ngc_args[i]);
	printf("\n");
}

char *wipe_cmd(int p, int *argc, char ***argv)
{
	char *ret;
	int i;

	ret = *argv[p];

	*argc--;
	for (i = p; i < *argc; i++)
		*argv[i] = *argv[i + 1];

	return (ret);
}

/* here is main */
int main(int argc, char *argv[])
{
	char path[1025];			/* the argv[0] is not always the full path, so we make a full path and put in here */
	char *new_argv[24];			/* used for execve, 24 arguments is really enoght */
	char *servname;				/* local storage of the service name, cut from / pointing in path abow */
	struct stat st;				/* file stat storage, used to check that files exist */

	/* check to no of arguments */
	if (argc < 3 || argc > 4)
	{
		int i;
		printf("Bad command: ");
		for(i = 0; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("\n");
		print_usage();
		exit(1);
	}

	if (argc == 4)
		wrapper = wipe_cmd(1, &argc, &argv);

	/* replace starting '.' with full path to local cwd */
	if (argv[1][0] == '.')
	{
		if (!getcwd(path, 1024))
		{
			printf("Cud not get path to pwd.\n");
			print_usage();
			exit(1);
		}
		strncat(path, &argv[1][1], 1024 - strlen(path));
	}
	/* replace starting '~' with path to HOME */
	else if (argv[1][0] == '~')
	{
		strncpy(path, getenv("HOME"), 1024);
		strncpy(path, &argv[1][1], 1024 - strlen(path));
	}
	/* if it is a full path, this is really good */
	else if (argv[1][0] == '/')
	{
		strncpy(path, argv[1], 1024);
	}
	/* else, guess the full path */
	else
	{
		strcpy(path, INITNG_ROOT);
		strncat(path, argv[1], 1024 - strlen(path));
	}

	/* check that path is correct */
	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
	{
		printf("Full path not provided, Guessed path to \"%s\" but no file existed in that place.\n", path);
		print_usage();
		exit(2);
	}

	/* cut service name from the last '/' found in service path */
	servname = getenv("SERVICE");
	if (!servname)
		servname = strrchr(path, '/') + 1;
	if (!servname)
	{
		printf("SERVICE is not known!\n");
		exit(3);
	}

	/* check if command shud forward to a ngc command */
	{
		int i;

		for (i = 0; ngc_args[i]; i++)
		{

			/* check if these are direct commands, then use ngc */
			if (strcmp(argv[2], ngc_args[i]) == 0)
			{
				/* set up an arg like "/sbin/ngc --start service" */
				new_argv[0] = (char *) "/sbin/ngc";
				/* put new_argv = "--start" */
				new_argv[1] = calloc(strlen(ngc_args[i] + 4), sizeof(char));
				new_argv[1][0] = '-';
				new_argv[1][1] = '-';
				strcat(new_argv[1], ngc_args[i]);
				/* put service name */
				new_argv[2] = servname;
				new_argv[3] = NULL;

				/* execute this call */
				execve(new_argv[0], new_argv, environ);
				printf("/sbin/ngc is missing or invalid.\n");
				exit(30);
			}
		}
	}
	/* end check */

	/* check if command is valid */
	if (strncmp(argv[2], "internal_", 9) != 0)
	{
		int i;
		printf("Bad command: ");
		for(i = 0; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("\n");
		print_usage();
		exit(3);
	}

	/* set up new argv */
	new_argv[0] = malloc(sizeof(WRAPPER_PATH) + strlen(wrapper));
	strcpy(new_argv[0], WRAPPER_PATH);
	strcat(new_argv[0], wrapper);
	new_argv[1] = NULL;

	/* set up the environments */
	setenv("SERVICE_FILE", path, 1);
	setenv("SERVICE", servname, 1);
	setenv("COMMAND", &argv[2][9], 1);

	/* now call the wrapper */
	execve(new_argv[0], new_argv, environ);

	/* Newer get here */
	printf("error executing /bin/sh.\n");
	exit(3);
}
