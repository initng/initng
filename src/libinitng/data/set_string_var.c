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

#include <initng.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "local.h"

/* this has to be only one of */
void initng_data_set_string_var(s_entry * type, char *vn, data_head * d,
				char *string)
{
	s_data *current = NULL;

	assert(d);
	assert(string);

	if (!type) {
		F_("Type can't be zero!\n");
		return;
	}

	ALIAS_WALK;

	if (!vn && type->type >= 50) {
		F_("The vn variable is missing for a type %i %s, trying to "
		   "set \"%s\"!\n", type->type, type->name, string);
		return;
	}

	if (!IT(STRING)) {
		F_(" \"%s\" is not an STRING type, sleeping 1 sec ..\n",
		   type->name);
		sleep(1);
		return;
	}

	/* check the db, for an current entry to overwrite */
	if ((current = initng_data_get_next_var(type, vn, d, NULL))) {
		if (current->t.s)
			free(current->t.s);
		if (vn)
			free(vn);
		current->t.s = string;
		return;
	}

	current = (s_data *) initng_toolbox_calloc(1, sizeof(s_data));
	current->type = type;
	current->t.s = string;
	current->vn = vn;

	/* add this one */
	initng_list_add(&(current->list), &d->head.list);
}
