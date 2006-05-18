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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "initng.h"
#include "initng_global.h"
#include "initng_toolbox.h"
#include "initng_plugin.h"
#include "initng_plugin_hook.h"
void initng_plugin_hook_unregister_real(const char *from_file,
										const char *func, int line,
										s_call * t, void *hook)
{
	s_call *current, *safe = NULL;

	assert(hook);
	assert(t);

	D_("Deleting hook from file %s, func %s, line %i.\n", from_file, func,
	   line);

	while_list_safe(current, t, safe)
	{
		/* make sure the pointer is right */
		if (current->c.pointer != hook)
			continue;

		list_del(&current->list);

		if (current->from_file)
			free(current->from_file);

		free(current);
		return;
	}

	F_("Could not find hook to delete, file: %s, func:%s, line %i!!!!\n",
	   from_file, func, line);
}

int initng_plugin_hook_register_real(const char *from_file, s_call * t,
									 int order, void *hook)
{
	s_call *new_call = NULL;
	s_call *current = NULL;

	assert(hook);
	assert(t);

	D_("\n\nAdding hook type %i from file %s, order %i\n", t, from_file,
	   order);

	/* allocate space for new call */
	new_call = i_calloc(1, sizeof(s_call));

	/* add data to call struct */
	new_call->order = order;
	new_call->from_file = i_strdup(from_file);
	new_call->c.pointer = hook;


	/* if list is empty, we don't need to care about order */
	if (list_empty(&t->list))
	{
		list_add(&new_call->list, &t->list);
		D_("Hook added to list successfully, as first.\n");
		return (TRUE);
	}


	/* fast forward, we wanna add hooks in order. */
	while_list(current, t)
	{
		/*D_("from %s order: %i\n", current->from_file, current->order); */
		if (current->order < order)
			continue;
		break;
	}

	/* add it in order */
	list_add(&new_call->list, &current->list);
	return (TRUE);
}
