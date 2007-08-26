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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <initng.h>

/* look for a command by command_id */
s_command *initng_command_find_by_command_id(char cid)
{
	s_command *current = NULL;

	while_command_db(current) {
		if (current->id == cid)
			return current;
	}
	return NULL;
}

/* look for a command by command_id */
s_command *initng_command_find_by_command_string(char *name)
{
	s_command *current = NULL;

	while_command_db(current) {
		if (current->long_id && strcmp(current->long_id, name) == 0)
			return current;
	}
	return NULL;
}
