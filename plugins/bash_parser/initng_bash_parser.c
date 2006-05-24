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

#include <initng.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

#include <initng_global.h>
#include <initng_active_state.h>
#include <initng_active_db.h>
#include <initng_process_db.h>
#include <initng_service_cache.h>
#include <initng_handler.h>
#include <initng_active_db.h>
#include <initng_toolbox.h>
#include <initng_plugin_hook.h>
#include <initng_load_module.h>
#include <initng_plugin_callers.h>
#include <initng_error.h>
#include <initng_plugin.h>
#include <initng_static_states.h>
#include <initng_control_command.h>

#include <initng-paths.h>

#include "initng_bash_parser.h"

INITNG_PLUGIN_MACRO;

static void bp_incomming(f_module_h * from, e_fdw what);
static void bp_closesock(void);
static void bp_handle_client(int fd);
static int  bp_open_socket(void);
static void bp_check_socket(int signal);
static void bp_new_active(bp_rep * rep, const char * type, const char * service);
static void bp_set_variable(bp_rep * rep, const char * service, const char * variable, const char * value);
static void bp_get_variable(bp_rep * rep, const char * service, const char * variable);

#define SOCKET_4_ROOTPATH "/dev/initng"

a_state_h PARSING = { "PARSING", "This is service is parsing by bash_parser.", IS_DOWN, NULL, NULL, NULL };


/*
   In the Linux implementation, sockets which are visible in the file system
   honour the permissions of the directory they are in.  Their owner, group
   and their permissions can be changed.  Creation of a new socket will
   fail if the process does not have write and search (execute) permission
   on the directory the socket is created in.  Connecting to the socket
   object requires read/write permission.  This behavior differs from  many
   BSD derived systems which ignore permissions for Unix sockets.  Portable
   programs should not rely on this feature for security.
 */


/* globals */
struct stat sock_stat;
f_module_h bpf = { &bp_incomming, FDW_READ, -1 };

#define RSCV() (TEMP_FAILURE_RETRY(recv(fd, &req, sizeof(bp_req), 0)) == (signed) sizeof(bp_req))
#define SEND() (send(fd, &rep, sizeof(bp_rep), 0) == (signed) sizeof(bp_rep))


static void bp_handle_client(int fd)
{
	bp_req req;
	bp_rep rep;
	printf("Got connection\n");
	memset(&req, 0, sizeof(bp_req));
	memset(&rep, 0, sizeof(bp_rep));

	/* use file descriptor, because fread hangs here? */
	if (!RSCV())
	{
		F_("Could not read incomming bash_parser req.\n");
		strcpy(rep.message, "Unable to read request");
		rep.success = FALSE;
		SEND();
		return;
	}
	printf("Got a request: ver: %i, type: %i\n", req.version, req.request);

	/* check protocol version match */
	if(req.version != BASH_PARSER_VERSION)
	{
		strcpy(rep.message, "Bad protocol version");
		rep.success = FALSE;
		SEND();
		return;
	}

	/* handle by request type */
	switch(req.request)
	{
		case NEW_ACTIVE:
			bp_new_active(&rep, req.u.new_active.type,
								req.u.new_active.service);
			break;
		case SET_VARIABLE:
			bp_set_variable(&rep, req.u.set_variable.service,
								  req.u.set_variable.variable,
								  req.u.set_variable.value);
			break;
		case GET_VARIABLE:
			bp_get_variable(&rep, req.u.get_variable.service,
								  req.u.get_variable.variable);
			break;
		default:
			break;
	}
	
	/* send the reply */
	SEND();
}

static void bp_new_active(bp_rep * rep, const char * type, const char *service)
{
	active_db_h * new_active;
	stype_h * stype;
	printf("bp_new_active(%s, %s)\n", type, service);
	
	/* find service type */
	stype = initng_service_type_get_by_name(type);
	if(!stype)
	{
		strcpy(rep->message, "Unable to find servicetype.");
		rep->success = FALSE;
		return;
	}
	
	/* create new service */
	new_active=initng_active_db_new(service);
	if(!new_active)
	{
		strcpy(rep->message, "Unable to create new service.");
		rep->success = FALSE;
		return;
	}
	
	/* set type */
	new_active->type = stype;
	new_active->current_state = &PARSING;
	
	/* register it */
	if(!initng_active_db_register(new_active))
	{
		strcpy(rep->message, "Unableto register new service, maby duplet?");
		rep->success = FALSE;
		return;
	}
	
	
	rep->success = TRUE;
	return;
}
static void bp_set_variable(bp_rep * rep, const char * service, const char * variable, const char * value)
{
	printf("bp_set_variable(%s, %s, %s)\n", service, variable, value);

	rep->success = TRUE;
	return;
}
static void bp_get_variable(bp_rep * rep, const char * service, const char * variable)
{
	printf("bp_get_variable(%s, %s)\n", service, variable);

	rep->success = TRUE;
	return;
}


/* called by fd hook, when data is no socket */
void bp_incomming(f_module_h * from, e_fdw what)
{
	int newsock;

	/* make a dumb check */
	if (from != &bpf)
		return;

	D_("bp_incomming();\n");
	/* we try to fix socket after every service start
	   if it fails here chances are a user screwed it
	   up, and we shouldn't manually try to fix anything. */
	if (bpf.fds <= 0)
	{
		/* socket is invalid but we were called, call closesocket to make sure it wont happen again */
		F_("accepted client called with bpf.fds %d, report bug\nWill repopen socket.", bpf.fds);
		bp_open_socket();
		return;
	}

	/* create a new socket, for reading */
	if ((newsock = accept(bpf.fds, NULL, NULL)) > 0)
	{
		/* read the data, by the handle_client function */
		bp_handle_client(newsock);

		/* close the socket when finished */
		close(newsock);
		return;
	}

	/* temporary unavailable */
	if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
	{
		W_("errno = EAGAIN!\n");
		return;
	}

	/* This'll generally happen on shutdown, don't cry about it. */
	D_("Error accepting socket %d, %s\n", bpf.fds, strerror(errno));
	bp_closesock();
	return;
}


/* this will try to open a new socket */
static int bp_open_socket()
{
	/*    int flags; */
	struct sockaddr_un serv_sockname;

	D_("Creating " SOCKET_4_ROOTPATH " dir\n");

	bp_closesock();

	/* Make /dev/initng if it doesn't exist (try either way) */
	if (mkdir(SOCKET_4_ROOTPATH, S_IRUSR | S_IWUSR | S_IXUSR) == -1
		&& errno != EEXIST)
	{
		if (errno != EROFS)
			F_("Could not create " SOCKET_4_ROOTPATH
			   " : %s, may be / fs not mounted read-write yet?, will retry until I succeed.\n",
			   strerror(errno));
		return (FALSE);
	}

	/* chmod root path for root use only */
	if (chmod(SOCKET_4_ROOTPATH, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
	{
		/* path doesn't exist, we don't have /dev yet. */
		if (errno == ENOENT || errno == EROFS)
			return (FALSE);

		F_("CRITICAL, failed to chmod %s, THIS IS A SECURITY PROBLEM.\n",
		   SOCKET_4_ROOTPATH);
	}

	/* Create the socket. */
	bpf.fds = socket(PF_UNIX, SOCK_STREAM, 0);
	if (bpf.fds < 1)
	{
		F_("Failed to init socket (%s)\n", strerror(errno));
		bpf.fds = -1;
		return (FALSE);
	}

	/* Bind a name to the socket. */
	serv_sockname.sun_family = AF_UNIX;

	strcpy(serv_sockname.sun_path, SOCKET_PATH);

	/* Remove old socket file if any */
	unlink(serv_sockname.sun_path);

	/* Try to bind */
	if (bind
		(bpf.fds, (struct sockaddr *) &serv_sockname,
		 (strlen(serv_sockname.sun_path) +
		  sizeof(serv_sockname.sun_family))) < 0)
	{
		F_("Error binding to socket (errno: %d str: '%s')\n", errno,
		   strerror(errno));
		bp_closesock();
		unlink(serv_sockname.sun_path);
		return (FALSE);
	}

	/* chmod socket for root only use */
	if (chmod(serv_sockname.sun_path, S_IRUSR | S_IWUSR) == -1)
	{
		F_("CRITICAL, failed to chmod %s, THIS IS A SECURITY PROBLEM.\n",
		   serv_sockname.sun_path);
		bp_closesock();
		return (FALSE);
	}

	/* store sock_stat for checking if we need to recreate socket later */
	stat(serv_sockname.sun_path, &sock_stat);

	/* Listen to socket */
	if (listen(bpf.fds, 5))
	{
		F_("Error on listen (errno: %d str: '%s')\n", errno, strerror(errno));
		bp_closesock();
		unlink(serv_sockname.sun_path);
		return (FALSE);
	}

	return (TRUE);
}

/* this will check socket, and reopen on failure */
static void bp_check_socket(int signal)
{
	struct stat st;

	if (signal != SIGHUP)
		return;

	D_("Checking socket\n");

	/* Check if socket needs reopening */
	if (bpf.fds <= 0)
	{
		D_("bpf.fds not set, opening new socket.\n");
		bp_open_socket();
		return;
	}

	/* stat the socket, reopen on failure */
	memset(&st, 0, sizeof(st));
	if (stat(SOCKET_PATH, &st) < 0)
	{
		W_("Stat failed! Opening new socket.\n");
		bp_open_socket();
		return;
	}

	/* compare socket file, with the one that we know, reopen on failure */
	if (st.st_dev != sock_stat.st_dev || st.st_ino != sock_stat.st_ino
		|| st.st_mtime != sock_stat.st_mtime)
	{
		F_("Invalid socket found, reopening\n");
		bp_open_socket();
		return;
	}

	D_("Socket ok.\n");
	return;
}

static void bp_closesock(void)
{
	/* Check if we need to remove hooks */
	if (bpf.fds < 0)
		return;
	D_("bp_closesock %d\n", bpf.fds);

	/* close socket and set to 0 */
	close(bpf.fds);
	bpf.fds = -1;
}



int module_init(int api_version)
{
	D_("module_init(ngc2);\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	/* zero globals */
	bpf.fds = -1;
	memset(&sock_stat, 0, sizeof(sock_stat));

	D_("adding hook, that will reopen socket, for every started service.\n");
	initng_plugin_hook_register(&g.FDWATCHERS, 30, &bpf);
	initng_plugin_hook_register(&g.SIGNAL, 50, &bp_check_socket);
	initng_active_state_register(&PARSING);

	/* do the first socket directly */
	bp_open_socket();

	return (TRUE);
}


void module_unload(void)
{
	D_("module_unload(ngc2);\n");
	if (g.i_am != I_AM_INIT && g.i_am != I_AM_FAKE_INIT)
		return;

	/* close open sockets */
	bp_closesock();

	/* remove hooks */
	initng_plugin_hook_unregister(&g.FDWATCHERS, &bpf);
	initng_plugin_hook_unregister(&g.SIGNAL, &bp_check_socket);
	initng_active_state_unregister(&PARSING);
}
