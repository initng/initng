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

#define _GNU_SOURCE

#include <initng.h>
#include <initng-paths.h>

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


#include "initng_service_file.h"

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
}

#ifdef GLOBAL_SOCKET
static void bp_incoming(f_module_h * from, e_fdw what);
static int bp_open_socket(void);
static void bp_check_socket(int signal);
static void bp_closesock(void);
#endif

static int parse_new_service_file(s_event * event, char *file);

static void bp_handle_client(int fd);
static void bp_new_active(bp_rep * rep, const char *type,
			  const char *service, const char *from_file);
static void bp_set_variable(bp_rep * rep, const char *service,
			    const char *vartype, const char *varname,
			    const char *value);
static void bp_get_variable(bp_rep * rep, const char *service,
			    const char *vartype, const char *varname);
static void bp_done(bp_rep * rep, const char *service);
static void bp_abort(bp_rep * rep, const char *service);

static void handle_killed(active_db_h * service, process_h * process);

#define SOCKET_4_ROOTPATH DEVDIR "/initng"

a_state_h PARSING = {
	.name = "PARSING",
	.description = "This is service is parsing by service_file.",
	.is = IS_NEW,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h PARSING_FOR_START = {
	.name = "PARSING_FOR_START",
	.description = "This is service is parsing by service_file.",
	.is = IS_NEW,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h NOT_RUNNING = {
	.name = "NOT_RUNNING",
	.description = "When a service is parsed done, its marked DOWN ready "
	    "for starting.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h REDY_FOR_START = {
	.name = "REDY_TO_START",
	.description = "When a service is parsed done, its marked DOWN ready "
	    "for starting.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h PARSE_FAIL = {
	.name = "PARSE_FAIL",
	.description = "This parse process failed.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/* put null to the kill_handler here */
ptype_h parse = { "parse-process", &handle_killed };
stype_h unset = {
	.name = "unset",
	.description = "Service type is not set yet, are still parsing.",
	.hidden = FALSE,
	.start = NULL,
	.stop = NULL,
	.restart = NULL
};

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

#ifdef GLOBAL_SOCKET

f_module_h bpf = {
	.call_module = &bp_incoming,
	.what = FDW_READ,
	.fds = -1
};

static int bpf_handler(s_event * event)
{
	s_event_fd_watcher_data *data;

	assert(event);
	assert(event->data);

	data = event->data;

	switch (data->action) {
	case FDW_ACTION_CLOSE:
		if (bpf.fds > 0)
			close(bpf.fds);
		break;

	case FDW_ACTION_CHECK:
		if (bpf.fds <= 2)
			break;

		/* This is a expensive test, but better safe then sorry */
		if (!STILL_OPEN(bpf.fds)) {
			D_("%i is not open anymore.\n", bpf.fds);
			bpf.fds = -1;
			break;
		}

		FD_SET(bpf.fds, data->readset);
		data->added++;
		break;

	case FDW_ACTION_CALL:
		if (!data->added || bpf.fds <= 2)
			break;

		if (!FD_ISSET(bpf.fds, data->readset))
			break;

		bp_incoming(&bpf, FDW_READ);
		data->added--;
		break;

	case FDW_ACTION_DEBUG:
		if (!data->debug_find_what ||
		    strstr(__FILE__, data->debug_find_what)) {
			initng_string_mprintf(data->debug_out, " %i: Used by plugin: %s\n",
				bpf.fds, __FILE__);
		}
		break;
	}

	return TRUE;
}

#endif

#define RSCV() (TEMP_FAILURE_RETRY(recv(fd, &req, sizeof(bp_req), 0)))
#define SEND() send(fd, &rep, sizeof(bp_rep), 0)

static void bp_handle_client(int fd)
{
	bp_req req;
	bp_rep rep;
	int r;

	D_("Got connection\n");
	memset(&req, 0, sizeof(bp_req));
	memset(&rep, 0, sizeof(bp_rep));

	/* use file descriptor, because fread hangs here? */
	r = RSCV();

	/* make sure it has not closed */
	if (r == 0) {
		/* printf("Closing %i.\n", fd); */
		close(fd);
		return;
	}

	if (r != (signed)sizeof(bp_req)) {
		F_("Could not read incoming service_file req. (fd %i)\n", fd);
		strcpy(rep.message, "Unable to read request");
		rep.success = FALSE;
		SEND();
		return;
	}
	D_("Got a request: ver: %i, type: %i\n", req.version, req.request);

	/* check protocol version match */
	if (req.version != SERVICE_FILE_VERSION) {
		strcpy(rep.message, "Bad protocol version");
		rep.success = FALSE;
		SEND();
		return;
	}

	/* handle by request type */
	switch (req.request) {
	case NEW_ACTIVE:
		bp_new_active(&rep, req.u.new_active.type,
			      req.u.new_active.service,
			      req.u.new_active.from_file);
		break;

	case SET_VARIABLE:
		bp_set_variable(&rep, req.u.set_variable.service,
				req.u.set_variable.vartype,
				req.u.set_variable.varname,
				req.u.set_variable.value);
		break;

	case GET_VARIABLE:
		bp_get_variable(&rep, req.u.get_variable.service,
				req.u.get_variable.vartype,
				req.u.get_variable.varname);
		break;

	case DONE:
		bp_done(&rep, req.u.done.service);
		break;

	case ABORT:
		bp_abort(&rep, req.u.abort.service);

	default:
		break;
	}

	/* send the reply */
	SEND();
}

static void bp_new_active(bp_rep * rep, const char *type,
			  const char *service, const char *from_file)
{
	active_db_h *new_active;
	stype_h *stype;

	D_("bp_new_active(%s, %s)\n", type, service);

	/* find service type */
	stype = initng_service_type_get_by_name(type);
	if (!stype) {
		strcpy(rep->message, "Unable to find service type \"");
		strncat(rep->message, type, 500);
		strcat(rep->message, "\" .");
		rep->success = FALSE;
		return;
	}

	/* look for existing */
	new_active = initng_active_db_find_by_name(service);

	/* check for duplet, not parsing */
	if (new_active && new_active->current_state != &PARSING_FOR_START) {
		strcpy(rep->message, "Duplet found.");
		rep->success = FALSE;
		return;
	}

	/* if not found, create new service */
	if (!new_active) {
		/* create new service */
		new_active = initng_active_db_new(service);
		if (!new_active)
			return;

		/* set type */
		new_active->current_state = &PARSING;

		/* register it */
		if (!initng_active_db_register(new_active)) {
			initng_active_db_free(new_active);
			return;
		}
	}

	/* save orgin */
	set_string(&FROM_FILE, new_active, initng_toolbox_strdup(from_file));

	/* set service type */
	new_active->type = stype;

	/* exit with success */
	rep->success = TRUE;
}

static void bp_set_variable(bp_rep * rep, const char *service,
			    const char *vartype, const char *varname,
			    const char *value)
{
	s_entry *type;
	active_db_h *active = initng_active_db_find_by_name(service);

	/* make sure there are filled, or NULL, these variables are not strictly requierd */
	if (!varname || strlen(varname) < 1)
		varname = NULL;
	if (!value || strlen(value) < 1)
		value = NULL;

	/* but these are */
	if (strlen(service) < 1) {
		strcpy(rep->message, "Service missing.");
		rep->success = FALSE;
		return;
	}

	if (strlen(vartype) < 1) {
		strcpy(rep->message, "Vartype missing.");
		rep->success = FALSE;
		return;
	}

	D_("bp_set_variable(%s, %s, %s, %s)\n", service, vartype,
	   varname, value);

	if (!active) {
		strcpy(rep->message, "Service \"");
		strcat(rep->message, service);
		strcat(rep->message, "\" not found.");
		rep->success = FALSE;
		return;
	}

	if (active->current_state != &PARSING
	    && active->current_state != &PARSING_FOR_START) {
		strcpy(rep->message, "Please dont edit finished services.");
		rep->success = FALSE;
		return;
	}

	if (!(type = initng_service_data_type_find(vartype))) {
		strcpy(rep->message, "Variable entry \"");
		strcat(rep->message, vartype);
		strcat(rep->message, "\" not found.");
		rep->success = FALSE;
		return;
	}

	/* now we know if variable is required or not */
	if (!value) {
		if ((type->type != SET && type->type != VARIABLE_SET)) {
			strcpy(rep->message, "Value missing.");
			rep->success = FALSE;
			return;
		}
	}

	switch (type->type) {
	case STRING:
	case VARIABLE_STRING:
		set_string_var(type, varname ? initng_toolbox_strdup(varname)
			       : NULL, active, initng_toolbox_strdup(value));
		D_("string type - %s %s\n", type->name, value);
		break;

	case STRINGS:
	case VARIABLE_STRINGS:{
			char *new_st = NULL;

			while ((new_st = initng_string_dup_next_word(&value))) {
				set_another_string_var(type,
						       varname ?
						       initng_toolbox_strdup
						       (varname)
						       : NULL, active, new_st);
			}

			D_("strings type\n");
			break;
		}

	case SET:
	case VARIABLE_SET:
		D_("set type\n");
		set_var(type,
			varname ? initng_toolbox_strdup(varname) : NULL,
			active);
		break;

	case INT:
	case VARIABLE_INT:
		set_int_var(type,
			    varname ? initng_toolbox_strdup(varname) :
			    NULL, active, atoi(value));
		D_("int type\n");
		break;

	default:
		strcpy(rep->message, "Unknown data type.");
		rep->success = FALSE;
		return;
	}

	rep->success = TRUE;
}

static void bp_get_variable(bp_rep * rep, const char *service,
			    const char *vartype, const char *varname)
{
	s_entry *type;
	active_db_h *active = initng_active_db_find_by_name(service);

	/* This one is not required */
	if (strlen(varname) < 1)
		varname = NULL;

	/* but these are */
	if (strlen(service) < 1 || strlen(vartype) < 1) {
		strcpy(rep->message, "Variables missing.");
		rep->success = FALSE;
		return;
	}

	D_("bp_get_variable(%s, %s, %s)\n", service, vartype, varname);
	if (!active) {
		strcpy(rep->message, "Service \"");

		strncat(rep->message, service, 500);
		strcat(rep->message, "\" not found.");
		rep->success = FALSE;
		return;
	}

	type = initng_service_data_type_find(vartype);
	if (!type) {
		strcpy(rep->message, "Variable entry not found.");
		rep->success = FALSE;
		return;
	}

	/* depening on data type */
	switch (type->type) {
	case STRING:
	case VARIABLE_STRING:
		strncpy(rep->message,
			get_string_var(type, varname, active), 1024);
		break;

	case STRINGS:
	case VARIABLE_STRINGS:
		/* todo, will only return one */
		{
			s_data *itt = NULL;
			const char *tmp = NULL;

			while ((tmp = get_next_string_var(type, varname,
							  active, &itt))) {
				if (!rep->message[0]) {
					strcpy(rep->message, tmp);
					continue;
				}
				strcat(rep->message, " ");
				strncat(rep->message, tmp, 1024);
			}
		}
		break;

	case SET:
	case VARIABLE_SET:
		if (is_var(type, varname, active))
			rep->success = TRUE;
		else
			rep->success = FALSE;
		return;

	case INT:
	case VARIABLE_INT:
		{
			int var = get_int_var(type, varname, active);

			sprintf(rep->message, "%i", var);
		}
		break;

	default:
		strcpy(rep->message, "Unknown data type.");
		rep->success = FALSE;
		return;
	}

	rep->success = TRUE;
	return;
}

static void bp_done(bp_rep * rep, const char *service)
{
	active_db_h *active = initng_active_db_find_by_name(service);

	if (!active) {
		strcpy(rep->message, "Service not found.");
		rep->success = FALSE;
		return;
	}

	/* check that type is set */
	if (!active->type || active->type == &unset) {
		strcpy(rep->message,
		       "Type not set, please run iregister type service\n");
		rep->success = FALSE;
		return;
	}

	/* must set to a DOWN state, to be able to start */
	if (IS_MARK(active, &PARSING_FOR_START)) {
		if (initng_common_mark_service(active, &REDY_FOR_START)) {
			rep->success = initng_handler_start_service(active);
			return;
		}
	} else if (IS_MARK(active, &PARSING)) {
		rep->success = initng_common_mark_service(active, &NOT_RUNNING);
		return;
	}

	strcpy(rep->message, "Service is not in PARSING state, cant start.");
	rep->success = FALSE;
	return;

	/* This wont start it, it will be started by as a dependency for a
	 * other plugin */
}

static void bp_abort(bp_rep * rep, const char *service)
{
	active_db_h *active = initng_active_db_find_by_name(service);

	if (!active) {
		strcpy(rep->message, "Service not found.");
		rep->success = FALSE;
		return;
	}

	if (active->current_state != &PARSING &&
	    active->current_state != &PARSING_FOR_START) {
		strcpy(rep->message,
		       "Service is not in PARSING state, cant start.");
		rep->success = FALSE;
		return;
	}

	D_("bp_abort(%s)\n", service);

	rep->success = TRUE;
}

#ifdef GLOBAL_SOCKET
/* called by fd hook, when data is no socket */
void bp_incoming(f_module_h * from, e_fdw what)
{
	int newsock;

	/* make a dumb check */
	if (from != &bpf)
		return;

	D_("bp_incoming();\n");
	/* we try to fix socket after every service start
	   if it fails here chances are a user screwed it
	   up, and we shouldn't manually try to fix anything. */
	if (bpf.fds <= 0) {
		/* socket is invalid but we were called, call closesocket to
		 * make sure it wont happen again */
		F_("accepted client called with bpf.fds %d, report bug\n"
		   "Will repopen socket.", bpf.fds);
		bp_open_socket();
		return;
	}

	/* create a new socket, for reading */
	if ((newsock = accept(bpf.fds, NULL, NULL)) > 0) {
		/* read the data, by the handle_client function */
		bp_handle_client(newsock);

		/* close the socket when finished */
		close(newsock);
		return;
	}

	/* temporary unavailable */
	if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
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
	    && errno != EEXIST) {
		if (errno != EROFS) {
			F_("Could not create " SOCKET_4_ROOTPATH
			   " : %s, may be / fs not mounted read-write yet?, "
			   "will retry until I succeed.\n", strerror(errno));
		}

		return FALSE;
	}

	/* chmod root path for root use only */
	if (chmod(SOCKET_4_ROOTPATH, S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
		/* path doesn't exist, we don't have /dev yet. */
		if (errno == ENOENT || errno == EROFS)
			return FALSE;

		F_("CRITICAL, failed to chmod %s, THIS IS A SECURITY "
		   "PROBLEM.\n", SOCKET_4_ROOTPATH);
	}

	/* Create the socket. */
	bpf.fds = socket(PF_UNIX, SOCK_STREAM, 0);
	if (bpf.fds < 1) {
		F_("Failed to init socket (%s)\n", strerror(errno));
		bpf.fds = -1;
		return (FALSE);
	}

	initng_fd_set_cloexec(bpf.fds);

	/* Bind a name to the socket. */
	serv_sockname.sun_family = AF_UNIX;

	strcpy(serv_sockname.sun_path, SOCKET_PATH);

	/* Remove old socket file if any */
	unlink(serv_sockname.sun_path);

	/* Try to bind */
	if (bind(bpf.fds, (struct sockaddr *)&serv_sockname,
		 (strlen(serv_sockname.sun_path) +
		  sizeof(serv_sockname.sun_family))) < 0) {
		F_("Error binding to socket (errno: %d str: '%s')\n",
		   errno, strerror(errno));
		bp_closesock();
		unlink(serv_sockname.sun_path);
		return FALSE;
	}

	/* chmod socket for root only use */
	if (chmod(serv_sockname.sun_path, S_IRUSR | S_IWUSR) == -1) {
		F_("CRITICAL, failed to chmod %s, THIS IS A SECURITY "
		   "PROBLEM.\n", serv_sockname.sun_path);
		bp_closesock();
		return FALSE;
	}

	/* store sock_stat for checking if we need to recreate socket later */
	stat(serv_sockname.sun_path, &sock_stat);

	/* Listen to socket */
	if (listen(bpf.fds, 5)) {
		F_("Error on listen (errno: %d str: '%s')\n", errno,
		   strerror(errno));
		bp_closesock();
		unlink(serv_sockname.sun_path);
		return FALSE;
	}

	return TRUE;
}
#endif

#ifdef GLOBAL_SOCKET
/* this will check socket, and reopen on failure */
static int bp_check_socket(s_event * event)
{
	int signal;
	struct stat st;

	assert(event->event_type == &EVENT_SIGNAL);

	signal = event->data;

	if (signal != SIGHUP)
		return TRUE;

	D_("Checking socket\n");

	/* Check if socket needs reopening */
	if (bpf.fds <= 0) {
		D_("bpf.fds not set, opening new socket.\n");
		bp_open_socket();
		return TRUE;
	}

	/* stat the socket, reopen on failure */
	memset(&st, 0, sizeof(st));
	if (stat(SOCKET_PATH, &st) < 0) {
		W_("Stat failed! Opening new socket.\n");
		bp_open_socket();
		return TRUE;
	}

	/* compare socket file, with the one that we know, reopen on
	 * failure */
	if (st.st_dev != sock_stat.st_dev
	    || st.st_ino != sock_stat.st_ino
	    || st.st_mtime != sock_stat.st_mtime) {
		F_("Invalid socket found, reopening\n");
		bp_open_socket();
		return TRUE;
	}

	D_("Socket ok.\n");
	return TRUE;
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
#endif

static void handle_killed(active_db_h * service, process_h * process)
{
	/* if process return code != 0, or the service was not
	 * registered, set fail */
	if (WEXITSTATUS(process->r_code) != 0 || service->type == &unset) {
		initng_common_mark_service(service, &PARSE_FAIL);
		initng_process_db_free(process);
		return;
	}

	initng_process_db_free(process);
	initng_handler_start_service(service);
}

static void create_new_active(s_event * event)
{
	char *r = NULL;
	char *file;
	char *name;
	int found = 0;

	assert(event->event_type == &EVENT_NEW_ACTIVE);
	assert(event->data);

	name = event->data;

	/* printf("create_new_active(%s);\n", name); */
	/* printf("service \"%s\" ", name); */

	/* "this" and "any" aren't allowed service names */
	{
		const char *bname = initng_string_basename(name);
		if (strcmp(bname, "this") == 0 || strcmp(bname, "any") == 0)
			return;
	}

	file = malloc(sizeof(INITNG_ROOT) + sizeof("/this") + strlen(name) + 2);

	/*
	 * scheme: "daemon/samba/smbd"
	 * try 1 "daemon/samba/smbd"
	 * try 2 "daemon/samba/smbd/this"
	 * try 3 "daemon/samba/any"
	 */

	strcpy(file, INITNG_ROOT "/");
	strcat(file, name);

	if (!(found = parse_new_service_file(event, file))) {
		r = strrchr(file, '/');

		/* Add "this" and see if there is better luck */
		strcat(file, "/this");
		if (!(found = parse_new_service_file(event, file)) && r) {
			r[0] = '\0';

			/* Add "any" */
			strcat(file, "/any");
			found = parse_new_service_file(event, file);
		}
	}

	if (found)
		event->status = HANDLED;

	free(file);
}

static int parse_new_service_file(s_event * event, char *file)
{
	char *name;
	active_db_h *new_active;
	process_h *process;
	pipe_h *current_pipe;
	struct stat fstat;

	name = event->data;

	/* Take stat on file */
	if (stat(file, &fstat) != 0)
		return FALSE;

	/* Is a regular file */
	if (!S_ISREG(fstat.st_mode))
		return FALSE;

	if (!(fstat.st_mode & S_IXUSR)) {
		F_("File \"%s\" can not be executed!\n", file);
		return FALSE;
	}

	/* create new service */
	new_active = initng_active_db_new(name);
	if (!new_active) {
		return FALSE;
	}

	/* set type */
	new_active->current_state = &PARSING_FOR_START;
	new_active->type = &unset;

	/* register it */
	if (!initng_active_db_register(new_active)) {
		initng_active_db_free(new_active);
		return FALSE;
	}

	/* create the process */
	process = initng_process_db_new(&parse);

	/* bound to service */
	add_process(process, new_active);

	/* create and unidirectional pipe */
	current_pipe = initng_process_db_pipe_new(IN_AND_OUT_PIPE);
	if (current_pipe) {
		/* we want this pipe to get fd 3, in the fork */
		current_pipe->targets[0] = 3;
		add_pipe(current_pipe, process);
	}

	/* start parse process */
	if (initng_fork(new_active, process) == 0) {
		char *new_argv[3];

		new_argv[0] = file;
		new_argv[1] = (char *)"internal_setup";
		new_argv[2] = NULL;

		execve(new_argv[0], new_argv, initng_env_new(new_active));
		_exit(10);
	}

	/* return the newly created */
	event->ret = new_active;
	return TRUE;
}

static void get_pipe(s_event * event)
{
	s_event_pipe_watcher_data *data;

	assert(event->event_type == &EVENT_PIPE_WATCHER);
	assert(event->data);

	data = event->data;

	/* printf("get_pipe(%s, %i, %i);\n", data->service->name,
	 * data->pipe->dir, data->pipe->targets[0]); */

	/* extra check */
	if (data->pipe->dir != IN_AND_OUT_PIPE)
		return;

	/* the pipe we opened was on fd 3 */
	if (data->pipe->targets[0] != 3)
		return;

	/* handle the client in the same way, as a fifo connected one */
	bp_handle_client(data->pipe->pipe[1]);

	/* return happy */
	event->status = HANDLED;
}

int module_init(int api_version)
{
	D_("module_init(ngc2);\n");
	if (api_version != API_VERSION) {
		F_("This module is compiled for api_version %i version and "
		   "initng is compiled on %i version, won't load this "
		   "module!\n", API_VERSION, api_version);
		return FALSE;
	}
#ifdef GLOBAL_SOCKET
	/* zero globals */
	bpf.fds = -1;
	memset(&sock_stat, 0, sizeof(sock_stat));
#endif

	D_("adding hook, that will reopen socket, for every started "
	   "service.\n");
	initng_process_db_ptype_register(&parse);
#ifdef GLOBAL_SOCKET
	initng_event_hook_register(&EVENT_FD_WATCHER, &bpf_handler);
	initng_event_hook_register(&EVENT_SIGNAL, &bp_check_socket);
#endif
	initng_event_hook_register(&EVENT_NEW_ACTIVE, &create_new_active);
	initng_event_hook_register(&EVENT_PIPE_WATCHER, &get_pipe);
	initng_active_state_register(&REDY_FOR_START);
	initng_active_state_register(&NOT_RUNNING);
	initng_active_state_register(&PARSING);
	initng_active_state_register(&PARSE_FAIL);

#ifdef GLOBAL_SOCKET
	/* do the first socket directly */
	bp_open_socket();
#endif

	return TRUE;
}

void module_unload(void)
{
	D_("module_unload(ngc2);\n");

#ifdef GLOBAL_SOCKET
	/* close open sockets */
	bp_closesock();
#endif

	/* remove hooks */
	initng_process_db_ptype_unregister(&parse);
#ifdef GLOBAL_SOCKET
	initng_event_hook_unregister(&EVENT_FD_WATCHER, &bpf_handler);
	initng_event_hook_unregister(&EVENT_SIGNAL, &bp_check_socket);
#endif
	initng_event_hook_unregister(&EVENT_NEW_ACTIVE, &create_new_active);
	initng_event_hook_unregister(&EVENT_PIPE_WATCHER, &get_pipe);
	initng_active_state_unregister(&REDY_FOR_START);
	initng_active_state_unregister(&NOT_RUNNING);
	initng_active_state_unregister(&PARSING);
	initng_active_state_unregister(&PARSE_FAIL);
}
