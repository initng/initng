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

#include "initng_bash_parser.h"

int bp_send(bp_req * to_send);
int bp_new_active(const char *type, const char *service);
int bp_abort(const char *service);
int bp_done(const char *service);
int bp_get_variable(const char *service, const char *vartype,
					const char *varname);
int bp_set_variable(const char *service, const char *vartype,
					const char *varname, const char *value);
char *message;



int main(int argc, char **argv)
{
	int status = 99;

	/* cut path or so from name */
	char *argv0 = strrchr(argv[0], '/');

	if (!argv0)
		argv0 = argv[0];
	if (argv0[0] == '/')
		argv0++;

	/* printf("argc: %i argv[0]: %s :: %s\n", argc, argv[0], argv0); */

	/* Sort by the number of arguments */
	switch (argc)
	{
		case 2:
			/* abort service */
			if (strcmp(argv0, "abort") == 0 || strcmp(argv0, "iabort") == 0)
			{
				status = bp_abort(argv[1]);
				break;
			}

			/* done service */
			if (strcmp(argv0, "done") == 0 || strcmp(argv0, "idone") == 0)
			{
				status = bp_done(argv[1]);
				break;
			}
			break;

		case 3:
			/* get service chdir */
			if (strcmp(argv0, "get") == 0 || strcmp(argv0, "iget") == 0)
			{
				status = bp_get_variable(argv[1], argv[2], NULL);
				break;
			}

			/* register service_type test */
			if (strcmp(argv0, "register") == 0 ||
				strcmp(argv0, "iregister") == 0)
			{
				status = bp_new_active(argv[1], argv[2]);
				break;
			}
			break;

		case 4:
			/* get service exec start */
			if (strcmp(argv0, "get") == 0 || strcmp(argv0, "iget") == 0)
			{
				status = bp_get_variable(argv[1], argv[2], argv[3]);
				break;
			}
			break;

		case 5:
			/* set service chdir = /root */
			if (argv[3][0] == '=' && (strcmp(argv0, "set") == 0 ||
									  strcmp(argv0, "iset") == 0))
			{
				status = bp_set_variable(argv[1], argv[2], NULL, argv[4]);
				break;
			}
			break;

		case 6:
			/* set service exec start = /bin/true */
			if (argv[4][0] == '=' && (strcmp(argv0, "set") == 0 ||
									  strcmp(argv0, "iset") == 0))
			{
				status = bp_set_variable(argv[1], argv[2], argv[3], argv[5]);
				break;
			}
			break;
		default:
			break;
	}

	/* if still 99, print usage */
	if (status == 99)
	{
		printf("Avaible commands:\n");
		printf("./idone     <service>\n");
		printf("./iabort    <service>\n");
		printf("./iget      <service> <variable>\n");
		printf("./iregister <type>    <name>\n");
		printf("./iset      <service> <variable> <value>\n");
		exit(status);
	}

	if (message)
	{
		printf("%s\n", message);
		free(message);
	}

	/* invert result */
	exit(status == TRUE ? 0 : 1);
}

int bp_abort(const char *service)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = ABORT;

	strncpy(to_send.u.abort.service, service, 100);

	return (bp_send(&to_send));
}
int bp_done(const char *service)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = DONE;

	strncpy(to_send.u.done.service, service, 100);

	return (bp_send(&to_send));
}
int bp_get_variable(const char *service, const char *vartype,
					const char *varname)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = GET_VARIABLE;

	strncpy(to_send.u.get_variable.service, service, 100);
	strncpy(to_send.u.get_variable.vartype, vartype, 100);
	if (varname)
		strncpy(to_send.u.get_variable.varname, varname, 100);

	return (bp_send(&to_send));
}
int bp_set_variable(const char *service, const char *vartype,
					const char *varname, const char *value)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = SET_VARIABLE;

	strncpy(to_send.u.set_variable.service, service, 100);
	strncpy(to_send.u.set_variable.vartype, vartype, 100);

	if (varname)
		strncpy(to_send.u.set_variable.varname, varname, 100);

	strncpy(to_send.u.set_variable.value, value, 1024);

	return (bp_send(&to_send));
}


int bp_new_active(const char *type, const char *service)
{
	/* the request to send */
	bp_req to_send;

	memset(&to_send, 0, sizeof(bp_req));

	to_send.request = NEW_ACTIVE;
	strncpy(to_send.u.new_active.type, type, 40);
	strncpy(to_send.u.new_active.service, service, 100);

	return (bp_send(&to_send));
}

/* Open, Send, Read, Close */
int bp_send(bp_req * to_send)
{
	int sock = 3;  /* testing fd 3, that is the oficcial pipe to initng for this communication */
	int len;
	struct sockaddr_un sockname;

	/* the reply from initng */
	bp_rep rep;

	memset(&rep, 0, sizeof(bp_rep));

	assert(to_send);
	to_send->version = BASH_PARSER_VERSION;

	/* check if we can use fd 3 to talk to initng */
	if(fcntl(sock, F_GETFD)<0)
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
	
	/* Put it not to block, waiting for more data on rscv */
	{
		int cur = fcntl(sock, F_GETFL, 0);

		fcntl(sock, F_SETFL, cur | O_NONBLOCK);
	}

	/* make a quick sleep */
	usleep(200);

	/* send the request */
	if (send(sock, to_send, sizeof(bp_req), 0) < (signed) sizeof(bp_req))
	{
		message = strdup("Unable to send the request.");
		return (FALSE);
	}

	/* do a short sleep to give initng a chanse to reply */
	usleep(200);

	/* get the reply */
	if ((TEMP_FAILURE_RETRY(recv(sock, &rep, sizeof(bp_rep), 0)) <
		 (signed) sizeof(bp_rep)))
	{
		message = strdup("Did not get any reply.");
		return (FALSE);
	}

	/* close the socket, if its an own created one */
	if(sock != 3)
		close(sock);

	/* if initng leave us a message, save it */
	if (strlen(rep.message) > 1)
		message = strdup(rep.message);

	/* return happily */
	return (rep.success);
}
