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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <initng.h>
#include <initng-paths.h>

#define WRAPPER_PATH INITNG_MODULE_DIR "/wrappers/"

extern char **environ;

const char *wrapper = "default";

/* FIXME: Wee need to change the way we detect when to forward commands */
/* These commands will be forwarded to /sbin/ngc if issued */
const char *ngc_args[] = {
	"start",
	"stop",
	"restart",
	"zap",
	"status",
	NULL
};

/* FIXME: this shouldn't be done by us */
/* this lists the commands the service can be executed with directly */
static void print_usage(char *me)
{
	int i;

	printf("Usage: %s", me);

	for (i = 0; ngc_args[i]; i++)
		printf(" <%s>", ngc_args[i]);

	printf("\n");
}

/* here is main */
int main(int argc, char *argv[])
{
	char path[1025];	/* the argv[0] is not always the full path,
				 * so we make a full path and put in here */
	char *new_argv[24];	/* used for execve, 24 arguments is really
				 * enough */
	char *servname;		/* local storage of the service name, cut
				 * from / pointing in path abow */
	struct stat st;		/* file stat storage, used to check that
				 * files exist */

	if (argc != 3) {
		int i;
		printf("Bad command:");

		for (i = 1; i < argc; i++)
			printf(" %s", argv[i]);

		printf("\n");
		print_usage(argv[1]);
		exit(1);
	}

	{
		char *tmp;

		/* set the wrapper */
		tmp = strrchr(initng_string_basename(argv[1]), '.');
		if ((tmp && tmp != argv[1]))
			wrapper = ++tmp;
	}

	if (argv[1][0] == '/') {
		/* a full path, this is really good */
		strncpy(path, argv[1], 1024);
	} else if (argv[1][0] == '.') {
		/* replace starting '.' with full path to local cwd */
		if (!getcwd(path, 1024)) {
			printf("Cud not get path to pwd.\n");
			print_usage(argv[1]);
			exit(1);
		}
		strncat(path, &argv[1][1], 1024 - strlen(path));
	} else {
		/* guess the full path */
		strcpy(path, INITNG_ROOT);
		strncat(path, argv[1], 1024 - strlen(path));
	}

	/* check that path is correct */
	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
		printf("File \"%s\" doesn't exist.\n", path);
		print_usage(argv[1]);
		exit(2);
	}

	/* FIXME: Should we use $SERVICE to detect when to realay the command to ngc? */
	/*        We should never set this by ourselves... */
	servname = getenv("SERVICE");
	if (!servname) {
		servname = (char *)initng_string_basename(path);
		setenv("SERVICE", servname, 1);
		setenv("NAME", servname, 1);
	}

	/* check if command shud forward to a ngc command */
	{
		int i;

		for (i = 0; ngc_args[i]; i++) {
			/* check if these are direct commands, then use ngc */
			if (strcmp(argv[2], ngc_args[i]) == 0) {
				/* set up an arg like "/sbin/ngc --start service" */
				new_argv[0] = (char *)"/sbin/ngc";
				/* put new_argv = "--start" */
				new_argv[1] =
				    calloc(strlen(ngc_args[i] + 4),
					   sizeof(char));
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

	/* FIXME: This isn't needed too */
	/* check if command is valid */
	if (strncmp(argv[2], "internal_", 9) != 0) {
		int i;

		printf("Bad command: ");

		for (i = 0; argv[i]; i++)
			printf(" %s", argv[i]);

		printf("\n");
		print_usage(argv[1]);

		exit(3);
	}

	/* set up new argv */
	new_argv[0] = malloc(sizeof(WRAPPER_PATH) + strlen(wrapper));
	strcpy(new_argv[0], WRAPPER_PATH);
	strcat(new_argv[0], wrapper);
	new_argv[1] = &argv[2][9];
	new_argv[2] = NULL;

	/* set up the environments */
	setenv("SFILE", path, 1);

	/* now call the wrapper */
	execve(new_argv[0], new_argv, environ);

	/* Never get here */
	printf("error executing %s.\n", new_argv[0]);

	exit(3);
}
