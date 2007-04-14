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

#include "initng.h"

#include <stdio.h>
#include <stdlib.h>							/* free() exit() */
#include <string.h>
#include <assert.h>

#include "initng_handler.h"
#include "initng_global.h"
#include "initng_common.h"
#include "initng_toolbox.h"
#include "initng_static_data_id.h"
#include "initng_static_states.h"
#include "initng_env_variable.h"
#include "initng_static_event_types.h"

#include "initng_depend.h"

#include "local.h"

/* standard dep check , does service depends on check? */
int dep_on(active_db_h * service, active_db_h * check)
{
	s_data *current = NULL;

	assert(service);
	assert(service->name);
	assert(check);
	assert(check->name);

	/* walk all possible entrys, use get_next with NULL because we want both REQUIRE and NEED */
	while ((current = get_next(NULL, service, current)))
	{
		/* only intreseted in two types */
		if (current->type != &REQUIRE && current->type != &NEED
			&& current->type != &USE)
			continue;

		/* to be sure */
		if (!current->t.s)
			continue;

		if (strcmp(current->t.s, check->name) == 0)
			return (TRUE);
	}

	/* No, it did not */
	return (FALSE);
}
