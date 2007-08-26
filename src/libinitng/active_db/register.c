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

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <assert.h>
#include <fnmatch.h>
#include <sys/time.h>
#include <time.h>

#include <initng.h>

/* add active to data structure */
/* remember to free service, if this fails */
int initng_active_db_register(active_db_h * add_this)
{
	active_db_h *current = NULL;

	/* we have to get this data */
	assert(add_this);
	assert(add_this->name);

	/* Look for duplicate */
	if ((current = initng_active_db_find_by_name(add_this->name))) {
		/* TODO, should add_this bee freed? */
		W_("active_db_add(%s): duplicate here\n", add_this->name);
		return (FALSE);
	}

	list_add(&add_this->list, &g.active_database.list);

	return (TRUE);
}
