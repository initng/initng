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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "local.h"


/* this is a function to add a string, that it can exist many of */
void d_set_another_string_var(s_entry * type, char *vn, data_head * d,
							  char *string)
{
	s_data *current = NULL;

	assert(d);
	assert(string);

	if (!type)
	{
		F_("Type can't be zero!\n");
		return;
	}

	ALIAS_WALK;

	if (!vn && type->opt_type >= 50)
	{
		F_("The vn variable is missing for a type %i %s, trying to set string: \"%s\"!\n", type->opt_type, type->opt_name, string);
		return;
	}

	if (!IT(STRINGS))
	{
		F_(" \"%s\" is not an strings type!\n", type->opt_name);
		return;
	}

	current = (s_data *) i_calloc(1, sizeof(s_data));
	current->type = type;
	current->t.s = string;
	current->vn = vn;

	/* add this one */
	list_add(&(current->list), &d->head.list);
}
