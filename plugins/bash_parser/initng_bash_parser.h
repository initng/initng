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

/* incomming type */
typedef enum
{
	UNSET_INVALID = 0,
	NEW_ACTIVE = 1,
	SET_VARIABLE = 2,
	GET_VARIABLE = 3,
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
			char variable[101]; /* The variable try to set */
			char value[1024];	/* The option value trying to set */
		} set_variable;
	
		/* get value */
		struct
		{
			char service[101];	/* Service to get option from */
			char variable[101]; /* The variable try to get */
		} get_variable;
	} u;
} bp_req;

typedef struct
{
	int success;			/* mainly TRUE or FALSE */
	char message[1024];		/* used for transporting a string to the client */
} bp_rep;

#endif
