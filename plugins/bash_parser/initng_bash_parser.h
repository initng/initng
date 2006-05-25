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

#ifndef BASH_PARSER_H
#define BASH_PARSER_H

#define BASH_PARSER_VERSION 1
#define SOCKET_PATH "/dev/initng/bp"
#define SCRIPT_PATH "/etc/init"

/* incomming type */
typedef enum
{
	UNSET_INVALID = 0,
	NEW_ACTIVE = 1,
	SET_VARIABLE = 2,
	GET_VARIABLE = 3,
	DONE = 4,
	ABORT = 5,
} bp_req_type;
	

typedef struct
{
	int version;				/* protocl version */
	bp_req_type request;		/* Can be new service request, new daemon request, set option .... */
	union
	{
		/* new active */
		struct 
		{
			char type[41];		/* What service type this is */
			char service[101];	/* New service name */
		} new_active;
		
		/* set value */
		struct
		{
			char service[101];	/* Service to set option to */
			char vartype[101]; /* The variable try to set */
			char varname[101];  /* The local variable name */
			char value[1025];	/* The option value trying to set */
		} set_variable;
	
		/* get value */
		struct
		{
			char service[101];	/* Service to get option from */
			char vartype[101]; /* The variable try to get */
			char varname[101];	/* The local varname */
		} get_variable;
		
		/* parsing done, now start it */
		struct
		{
			char service[101];	/* the service that is done parsing */
		} done;
		
		/* abort parsing, and clear service */
		struct
		{
			char service[101];	/* name of service to abort */
		} abort;
	} u;
} bp_req;

typedef struct
{
	int success;			/* mainly TRUE or FALSE */
	char message[1025];		/* used for transporting a string to the client */
} bp_rep;

#endif
