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

#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <initng.h>

/* this adds a command to the global command struct */
int initng_command_register(s_command * cmd)
{
	s_command *current = NULL;

	assert(cmd);

	/* look for duplicates */
	while_command_db(current) {
		if (current == cmd || current->id == cmd->id) {
			F_("Can't add command: %c, %s, it exists already!\n",
			   current->id, current->description);
			return FALSE;
		}
	}

	/* add this command to list */
	list_add(&cmd->list, &g.command_db.list);
	return TRUE;
}

void initng_command_unregister_all(void)
{
	s_command *current, *safe = NULL;

	while_command_db_safe(current, safe) {
		initng_command_unregister(current);
	}
}
