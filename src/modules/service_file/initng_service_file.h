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

#ifndef SERVICE_FILE_H
#define SERVICE_FILE_H


#define SERVICE_FILE_VERSION 1
#define SOCKET_PATH CTLDIR "/bp"

/* incoming type */
typedef enum {
	UNSET_INVALID = 0,
	NEW_ACTIVE = 1,
	SET_VARIABLE = 2,
	GET_VARIABLE = 3,
	DONE = 4,
	ABORT = 5,
} bp_req_type;


typedef struct {
	int version;			/* protocol version */
	bp_req_type request;		/* Can be new service request, new
					 * daemon request, set option .... */

	char service[101];

	union {
		/* new active */
		struct {
			char type[41];		/* What service type this is */
			char from_file[101];	/* The source file added from */
		} new_active;

		/* set value */
		struct {
			char vartype[101];	/* The variable try to set */
			char varname[101];	/* The local variable name */
			char value[1025];	/* The option value trying to set */
		} set_variable;

		/* get value */
		struct {
			char vartype[101];	/* The variable try to get */
			char varname[101];	/* The local varname */
		} get_variable;
	} u;
} bp_req;


#define BP_REP_MAXLEN 1024

typedef struct {
	int success;				/* mainly TRUE or FALSE */
	char message[BP_REP_MAXLEN + 1];	/* used for transporting a string to the client */
} bp_rep;

#endif
