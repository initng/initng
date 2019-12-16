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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>

#include <sys/types.h>	/* unix domain sockets */
#include <sys/socket.h>
#include <sys/un.h>

#include <initng.h>
#include <initng-paths.h>

#include "initng_service_file.h"

#ifndef DEBUG_EXTRA
#define DEBUG_EXTRA 0
#endif


typedef struct {
	const char *name;
	int (*function) (bp_req *to_send, int argc, char **argv);
	bp_req_type req_type;
} command_entry;


static int call_command(command_entry *cmd, char *service, int argc,
			char **argv);

static int bp_send(bp_req * to_send);

/* these are gonna be used for main() for every command */

static int bp_trivial(bp_req *to_send, int argc, char **argv);
static int bp_new_active(bp_req *to_send, int argc, char **argv);
static int bp_get_variable(bp_req *to_send, int argc, char **argv);
static int bp_set_variable(bp_req *to_send, int argc, char **argv);
static int bp_add_exec(bp_req *to_send, int argc, char **argv);
static int unknown_command(bp_req *to_send, int argc, char **argv);

char *message;

enum command_map {
	IUNKNOWN = 0,
	IABORT,
	IREGISTER,
	IDONE,
	IGET,
	ISET,
	IEXEC,
};

command_entry commands[] = {
	[IUNKNOWN]  = {NULL,        &unknown_command, 0},
	[IABORT]    = {"iabort",    &bp_trivial,      ABORT},
	[IREGISTER] = {"iregister", &bp_new_active,   NEW_ACTIVE},
	[IDONE]     = {"idone",     &bp_trivial,      DONE},
	[IGET]      = {"iget",      &bp_get_variable, GET_VARIABLE},
	[ISET]      = {"iset",      &bp_set_variable, SET_VARIABLE},
	[IEXEC]     = {"iexec",     &bp_add_exec,     SET_VARIABLE},
};

static int unknown_command(bp_req *to_send, int argc, char **argv)
{
	printf("Bad command \"");
	for (int i = 0; argv[i]; i++)
		printf(" %s", argv[i]);
	printf("\"\nAvaible commands:\n");
	for (int i = 0; commands[i].name; i++)
		printf(" ./%s\n", commands[i].name);
	exit(99);
}

static command_entry *map_cmd(const char *cmd)
{
	int r;

	switch(cmd[1]) {
	case 'a':
		r = IABORT;
		break;
	case 'r':
		r = IREGISTER;
		break;
	case 'd':
		r = IDONE;
		break;
	case 'g':
		r = IGET;
		break;
	case 's':
		r = ISET;
		break;
	case 'e':
		r = IEXEC;
		break;
	default:
		goto unknown;
	}

	if (strcmp(commands[r].name, cmd) != 0) {
	unknown:
		r = IUNKNOWN;
	}

	return &commands[r];
}

static int call_command(command_entry *cmd, char *service, int argc,
			char **argv)
{
	bp_req to_send;

	if (cmd->req_type) {
		memset(&to_send, 0, sizeof(to_send));
		to_send.request = cmd->req_type;
		strncpy(to_send.service, service, 100);
	}

	return (cmd->function)(&to_send, argc, argv);
}


int main(int argc, char **argv)
{
	int status;
	int new_argc;

	const char *argv0 = initng_string_basename(argv[0]);

	/* allocate a new argv to use */
	char **new_argv = calloc(argc + 1, sizeof(char *));

	/* first is the full path to service file */
	new_argv[0] = getenv("SFILE");
	if (!new_argv[0]) {
		printf("SFILE path is unset!\n");
		exit(1);
	}

	/* get service */
	char *service = getenv("SERVICE");

	/* make sure */
	if (!service) {
		printf("I dont know what service you want!\n");
		exit(1);
	}

	/* copy all entries */
	new_argc = 0;
	for (int i = 1; argv[i]; i++) {
		new_argv[++new_argc] = argv[i];
	}

	if (DEBUG_EXTRA) {
		printf(" **  %-12s ** ", service);
		for (int i = 0; argv[i]; i++)
			printf("%s ", argv[i]);
		printf("\n");
	}

	command_entry *cmd = map_cmd(argv0);
	status = call_command(cmd, service, new_argc, new_argv);

	if (message) {
		printf("%s (%s) :", cmd->name, service);
		for (int i = 1; new_argv[i]; i++)
			printf(" %s", new_argv[i]);

		printf("  \"%s\"\n", message);
	}

	/* invert result */
	exit(!status);
}

/*
 * usage: iexec start           will run /etc/initng/service internal_start
 *        iexec start = dodo    will run /etc/initng/service internal_dodo
 */
static int bp_add_exec(bp_req *to_send, int argc, char **argv)
{
	if (argc != 1 && (argc != 3 || argv[2][0] != '='))
		return FALSE;

	int left = 1024;

	if (argc != 3 || argv[3][0] != '/') {
		/* "/etc/initng/file" */
		strncpy(to_send->u.set_variable.value, argv[0], left);
		left -= strlen(argv[0]);

		/* " internal_" */
		strncat(to_send->u.set_variable.value, " internal_", left);
		left -= strlen(" internal_");
	}

	if (argc == 1) {
		/* "start" */
		strncat(to_send->u.set_variable.value, argv[1], left);
		left -= strlen(argv[1]);
	} else {
		/* "dodo" */
		strncat(to_send->u.set_variable.value, argv[3], left);
		left -= strlen(argv[3]);
	}

	/* the type is "exec" */
	strncpy(to_send->u.set_variable.vartype, "exec", 100);

	/* the varname is "start" */
	strncpy(to_send->u.set_variable.varname, argv[1], 100);

	return bp_send(to_send);
}

static int bp_trivial(bp_req *to_send, int argc, char **argv)
{
	return bp_send(to_send);
}

/* This have 2 senarios, with 1 or 2 argc:
 *  iget test
 *  iget exec test
 */

static int bp_get_variable(bp_req *to_send, int argc, char **argv)
{
	/* make sure its 1 or 2 args with this */
	if (argc != 1 && argc != 2)
		return FALSE;

	if (argc == 2) {
		strncpy(to_send->u.get_variable.varname, argv[1], 100);
		argv++;
	}
	strncpy(to_send->u.get_variable.vartype, argv[1], 100);

	return bp_send(to_send);
}

/*
 * Have 4 usages:
 *  With only 1 arg and no '=', sets a variable without value.
 *  1) iset forks
 *  With only 2 args and no '=', sets a variable without value.
 *  2) iset start forks
 *  With minimum 3 words and 2nd a '='
 *  3) iset test = "Coool"
 *  With minimum 4 words and 3rd a '='
 *  4) iset exec test = "Coool"
 */

static int bp_set_variable(bp_req *to_send, int argc, char **argv)
{
	/* make sure have enough params */
	if (argc < 1)
		return FALSE;

	/* value-less variable */
	if (argc < 3) {
		/* type 1: type-only */
		strncpy(to_send->u.set_variable.vartype, argv[1], 100);

		/* type 2: type+name */
		if (argc == 2)
			strncpy(to_send->u.set_variable.varname, argv[2], 100);
		return bp_send(to_send);
	}

	strncpy(to_send->u.set_variable.vartype, argv[1], 100);

	int i;

	/* type 3: short set without varname */
	if (argv[2][0] == '=') {
		/* argv[2] == '=' */
		i = 3;
	}
	/* else type 4 */
	else if (argc >= 4 && argv[3][0] == '=') {
		strncpy(to_send->u.set_variable.varname, argv[2], 100);
		/* argv[3] == '=' */
		i = 4;
	} else
		return FALSE;

	for (; argv[i]; i++) {
		strncpy(to_send->u.set_variable.value, argv[i], 1024);
		if (!bp_send(to_send))
			return FALSE;
	}

	return TRUE;
}

static int bp_new_active(bp_req *to_send, int argc, char **argv)
{
	/* do a check */
	if (argc != 1)
		return FALSE;

	/* set the type */
	strncpy(to_send->u.new_active.type, argv[1], 40);
	strncpy(to_send->u.new_active.from_file, argv[0], 100);

	return bp_send(to_send);
}

/* Open, Send, Read, Close */
static int bp_send(bp_req * to_send)
{
	int sock = 3;		/* testing fd 3, that is the oficcial pipe to initng for this communication */
	int len;
	struct sockaddr_un sockname;
	int e;

	/* the reply from initng */
	bp_rep rep;

	memset(&rep, 0, sizeof(bp_rep));

	assert(to_send);
	to_send->version = SERVICE_FILE_VERSION;

	/* check if we can use fd 3 to talk to initng */
	if (fcntl(sock, F_GETFD) < 0) {

		/* ELSE Create new socket. */
		sock = socket(PF_UNIX, SOCK_STREAM, 0);
		if (sock < 0) {
			message = strdup("Failed to init socket.");
			return FALSE;
		}

		/* Bind a name to the socket. */
		sockname.sun_family = AF_UNIX;
		strcpy(sockname.sun_path, SOCKET_PATH);
		len = strlen(SOCKET_PATH) + sizeof(sockname.sun_family);

		if (connect(sock, (struct sockaddr *)&sockname, len) < 0) {
			close(sock);
			message = strdup("Error connecting to socket");
			return FALSE;
		}
	}

	/* send the request */
	e = send(sock, to_send, sizeof(bp_req), 0);
	if (e != (signed)sizeof(bp_req)) {
		char *m = strerror(errno);
		message = calloc(501, 1);
		snprintf(message, 500, "Unable to send the request: "
			 "\"%s\" (%i)\n", m, errno);
		return FALSE;
	}

	/* sleep to give initng a chanse */
	usleep(200);

	e = recv(sock, &rep, sizeof(bp_rep), 0);

	if (e != (signed)sizeof(bp_rep)) {
		char *m = strerror(errno);
		message = calloc(501, 1);
		snprintf(message, 500, "Did not get any reply: "
			 "\"%s\" (%i)\n", m, errno);
		return FALSE;
	}

	/* close the socket, if its an own created one */
	if (sock != 3)
		close(sock);

	/* if initng leave us a message, save it */
	if (strlen(rep.message) > 1)
		message = strdup(rep.message);

	/* return happily */
	return rep.success;
}
