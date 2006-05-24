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
int bp_new_active(const char * type, const char * service);
char *message;

int main(int argc, char **argv)
{
	int t = bp_new_active("service", "test");
	printf("Sent new active \"service\", \"test\"\n  result: %s\n  message: \"%s\"\n", t==TRUE ? "success" : "failure", message);
	
	if(message)
		free(message);
		
	exit(t);
}

int bp_new_active(const char * type, const char * service)
{
	/* the request to send */
	bp_req to_send;
	memset(&to_send, 0, sizeof(bp_req));
	
	to_send.version = BASH_PARSER_VERSION;
	to_send.request = NEW_ACTIVE;
	strncpy(to_send.u.new_active.type, type, 50);
	strncpy(to_send.u.new_active.service, service, 100);
	
	return(bp_send(&to_send));
}

/* Open, Send, Close */
int bp_send(bp_req * to_send)
{
	int sock = -1;
	int len;
	struct sockaddr_un sockname;
	/* the reply from initng */
	bp_rep rep;
	memset(&rep, 0, sizeof(bp_rep));


	/* Create the socket. */
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

	/* Put it not to block, waiting for more data on rscv */
	{
		int cur = fcntl(sock, F_GETFL, 0);

		fcntl(sock, F_SETFL, cur | O_NONBLOCK);
	}

	/* send the request */
	if(send(sock, &to_send, sizeof(bp_req), 0) < (signed) sizeof(bp_req))
	{
		message = strdup("Unable to send the request.");
		return(FALSE);
	}
	
	/* do a short sleep to give initng a chanse to reply */
	usleep(200);

	/* get the reply */
	if((TEMP_FAILURE_RETRY(recv(sock, &rep, sizeof(bp_rep), 0)) < (signed) sizeof(bp_rep)))
	{
		message = strdup("Did not get any reply.");
		return (FALSE);
	}
	
	/* close the socket */
	close(sock);
	
	/* if initng leave us a message, save it */
	if(strlen(rep.message)>1)
		message=strdup(rep.message);
		
	/* return happily */
	return (rep.success);
}

