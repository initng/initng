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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>

#include <initng.h>
#include <initng-paths.h>

#include "initng_service_file.h"

int bp_send(bp_req * to_send);

/* these are gonna be used for main() for every command */
int bp_new_active(char *service, int argc, char **argv);
int bp_abort(char *service, int argc, char **argv);
int bp_done(char *service, int argc, char **argv);
int bp_get_variable(char *service, int argc, char **argv);
int bp_set_variable(char *service, int argc, char **argv);
int bp_add_exec(char *service, int argc, char **argv);
char *message;

typedef struct
{
	const char *name;
	int (*function) (char *service, int argc, char **argv);
} command_entry;

command_entry commands[] = {
	{"iabort", &bp_abort},
	{"iregister", &bp_new_active},
	{"idone", &bp_done},
	{"iget", &bp_get_variable},
	{"iset", &bp_set_variable},
	{"iexec", &bp_add_exec},
	{NULL, NULL}
};

int main(int argc, char **argv)
{
	int status = 99;
	char *service = NULL;
	char **new_argv;
	int new_argc;
	int i;
	int stop_checking = FALSE;

	/* cut path or so from name */
	char *argv0 = strrchr(argv[0], '/');

	if (!argv0)
		argv0 = argv[0];
	if (argv0[0] == '/')
		argv0++;

	/* allocate a new argv to use */
	new_argv = calloc(argc + 1, sizeof(char *));

	/* first is the full path to service file */
	new_argv[0] = getenv("SERVICE_FILE");
	if (!new_argv[0])
	{
		printf("SERVICE_FILE path is unset!\n");
		exit(1);
	}

	/* copy all, but not options */
	new_argc = 0;
	for (i = 1; argv[i]; i++)
	{
		/* iset -s service test */
		if (stop_checking == FALSE && argv[i][0] == '-')
		{
			if (argv[i][1] == 's' && !argv[i][2] && argv[i + 1][0])
			{
				service = argv[i + 1];
				i++;
				continue;
			}

			printf("unknown variable \"%s\"\n", argv[i]);
		}
		/* stop searching for arguments behind the '=' char */
		if (argv[i][0] == '=')
			stop_checking = TRUE;

		/* copy the entry */
		new_argc++;
		new_argv[new_argc] = argv[i];
	}

	/* if service was not found.. */
	if (!service)
		service = getenv("SERVICE");

	/* make sure */
	if (!service)
	{
		printf("I dont know what service you want!\n");
		exit(1);
	}

#ifdef DEBUG_EXTRA
	{
		printf(" **  %-12s ** ", service);
		for (i = 0; argv[i]; i++)
			printf("%s ", argv[i]);
		printf("\n");
	}
#endif

	/* LIST THE DB OF COMMANDS AND EXECUTE THE RIGHT ONE */
	{
		for (i = 0; commands[i].name; i++)
		{
			if (strcasecmp(argv0, commands[i].name) == 0)
				status = (*commands[i].function) (service,
						new_argc, new_argv);
			if (status != 99)
				break;
		}
	}

	/* if still 99, print usage */
	if (status == 99)
	{
		printf("Bad command \"");
		for(i=0; argv[i];i++)
			printf(" %s", argv[i]);
		printf("\"\nAvaible commands:\n");
		for (i = 0; commands[i].name; i++)
			printf(" ./%s\n", commands[i].name);
		exit(status);
	}

	if (message)
	{
		printf("%s (%s) :", commands[i].name, service);
		for (i = 1; new_argv[i]; i++)
			printf(" %s", new_argv[i]);

		printf("  \"%s\"\n", message);
		free(message);
	}

	/* invert result */
	exit(status == TRUE ? 0 : 1);
}

/*
 * usage: iexec start           will run /etc/initng/service internal_start
 *        iexec start = dodo    will run /etc/initng/service internal_dodo
 */
int bp_add_exec(char *service, int argc, char **argv)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));
	to_send.request = SET_VARIABLE;


	if (argc == 1 || (argc == 3 && argv[2][0] == '=')) {
		if (argc != 3 || argv[3][0] != '/') {
			/* "/etc/initng/file" */
			strncpy(to_send.u.set_variable.value, argv[0], 1024);

			/* " internal_" */
			strncat(to_send.u.set_variable.value, " internal_",
					1024 - strlen(to_send.u.set_variable.value));
		}

		if (argc == 1) {
			/* "start" */
			strncat(to_send.u.set_variable.value, argv[1],
					1024 - strlen(to_send.u.set_variable.value));
		}
		else
		{
			/* "dodo" */
			strncat(to_send.u.set_variable.value, argv[3],
					1024 - strlen(to_send.u.set_variable.value));
		}
	}
	else
		return (FALSE);

	/* use service */
	strncpy(to_send.u.get_variable.service, service, 100);

	/* the type is "exec" */
	strcpy(to_send.u.set_variable.vartype, "exec");

	/* the varname is "start" */
	strncpy(to_send.u.set_variable.varname, argv[1], 100);

	return (bp_send(&to_send));
}

int bp_abort(char *service, int argc, char **argv)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = ABORT;

	strncpy(to_send.u.abort.service, service, 100);

	return (bp_send(&to_send));
}

int bp_done(char *service, int argc, char **argv)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = DONE;

	strncpy(to_send.u.done.service, service, 100);

	return (bp_send(&to_send));
}

/* This have 2 senarios, with 1 or 2 argc:
 *  iget test
 *  iget exec test
 */

int bp_get_variable(char *service, int argc, char **argv)
{
	/* the request to send */
	bp_req to_send;

	/* make sure its 1 or 2 args with this */
	if (argc != 1 && argc != 2)
		return (FALSE);

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = GET_VARIABLE;

	/* use service */
	strncpy(to_send.u.get_variable.service, service, 100);


	if (argc == 1)
	{
		strncpy(to_send.u.get_variable.vartype, argv[1], 100);
	}
	else
	{
		strncpy(to_send.u.get_variable.varname, argv[1], 100);
		strncpy(to_send.u.get_variable.vartype, argv[2], 100);
	}

	return (bp_send(&to_send));
}

/*
 * Have 4 usages:
 *  With only 1 arg and no '=', sets a variable without value.
 *  1) iset forks
 *  With only 2 args and no '=', sets a variable without value.
 *  2) iset start forks
 *  With minimum 3 words and 2on a '='
 *  3) iset test = "Coool"
 *  With minimum 4 words and 3rd a '='
 *  4) iset exec test = "Coool"
 */

int bp_set_variable(char *service, int argc, char **argv)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));
	to_send.request = SET_VARIABLE;
	int i;
	int ret = FALSE;

	/* make sure have enought variables */
	if (argc < 1)
		return (FALSE);

	/* use service set in main() */
	strncpy(to_send.u.set_variable.service, service, 100);

	/* if not usage 3 or 4 */
	if (argc < 3)
	{
		/* handle valueless variable, type 1 */
		if (argc == 1)
		{
			strncpy(to_send.u.set_variable.vartype, argv[1], 100);
			return (bp_send(&to_send));
		}

		/* handle valueless variable, type 2 */
		if (argc == 2)
		{
			strncpy(to_send.u.set_variable.vartype, argv[1], 100);
			strncpy(to_send.u.set_variable.varname, argv[2], 100);
			return (bp_send(&to_send));
		}

		/* then this is not valid */
		return (FALSE);
	}

	/* if its a short set ( type 3 )  without vartype */
	if (argc >= 3 && argv[2][0] == '=')
	{
		strncpy(to_send.u.set_variable.vartype, argv[1], 100);
		/* argv[2] == '=' */
		for (i = 3; argv[i]; i++)
		{
			strncpy(to_send.u.set_variable.value, argv[i], 1024);
			ret = bp_send(&to_send);
		}
		return (ret);
	}

	/* else type 4 */
	if (argc >= 4 && argv[3][0] == '=')
	{
		strncpy(to_send.u.set_variable.vartype, argv[1], 100);
		strncpy(to_send.u.set_variable.varname, argv[2], 100);
		/* argv[3] == '=' */
		for (i = 4; argv[i]; i++)
		{
			strncpy(to_send.u.set_variable.value, argv[i], 1024);
			ret = bp_send(&to_send);
		}
		return (ret);
	}

	return (FALSE);
}


int bp_new_active(char *service, int argc, char **argv)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));
	to_send.request = NEW_ACTIVE;

	/* do a check */
	if (argc != 1)
		return (FALSE);

	/* use servicename from main() */
	strncpy(to_send.u.new_active.service, service, 100);

	/* set the type */
	strncpy(to_send.u.new_active.type, argv[1], 40);
	strncpy(to_send.u.new_active.from_file, argv[0], 100);


	return (bp_send(&to_send));
}

/* Open, Send, Read, Close */
int bp_send(bp_req * to_send)
{
	int sock = 3;				/* testing fd 3, that is the oficcial pipe to initng for this communication */
	int len;
	struct sockaddr_un sockname;
	int e;

	/* the reply from initng */
	bp_rep rep;

	memset(&rep, 0, sizeof(bp_rep));

	assert(to_send);
	to_send->version = SERVICE_FILE_VERSION;

	/* check if we can use fd 3 to talk to initng */
	if (fcntl(sock, F_GETFD) < 0)
	{

		/* ELSE Create new socket. */
		sock = socket(PF_UNIX, SOCK_STREAM, 0);
		if (sock < 0)
		{
			message = strdup("Failed to init socket.");
			return (FALSE);
		}

		/* Bind a name to the socket. */
		sockname.sun_family = AF_UNIX;
		strcpy(sockname.sun_path, SOCKET_PATH);
		len = strlen(SOCKET_PATH) + sizeof(sockname.sun_family);

		if (connect(sock, (struct sockaddr *) &sockname, len) < 0)
		{
			close(sock);
			message = strdup("Error connecting to socket");
			return (FALSE);
		}
	}

	/* send the request */
	e = send(sock, to_send, sizeof(bp_req), 0);
	if (e != (signed) sizeof(bp_req))
	{
		char *m = strerror(errno);
		message = calloc(501, sizeof(char));
		snprintf(message, 500, "Unable to send the request: \"%s\" (%i)\n", m,
				 errno);
		return (FALSE);
	}

	/* sleep to give initng a chanse */
	usleep(200);

	e = TEMP_FAILURE_RETRY(recv(sock, &rep, sizeof(bp_rep), 0));



	if (e != (signed) sizeof(bp_rep))
	{
		char *m = strerror(errno);
		message = calloc(501, sizeof(char));
		snprintf(message, 500, "Did not get any reply: \"%s\" (%i)\n", m,
				 errno);
		return (FALSE);
	}

	/* close the socket, if its an own created one */
	if (sock != 3)
		close(sock);

	/* if initng leave us a message, save it */
	if (strlen(rep.message) > 1)
		message = strdup(rep.message);

	/* return happily */
	return (rep.success);
}
