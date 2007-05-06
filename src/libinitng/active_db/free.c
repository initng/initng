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


/* free some values in this one */
void initng_active_db_free(active_db_h * pf)
{
	process_h *current = NULL;
	process_h *safe = NULL;

	assert(pf);
	assert(pf->name);

	D_("(%s);\n", pf->name);

	/* look if there are plug-ins, that is interested to now, this is freeing */
	initng_common_mark_service(pf, &FREEING);

	/* unregister on all lists */
	list_del(&pf->list);
	list_del(&pf->interrupt);

	while_processes_safe(current, safe, pf)
	{
		initng_process_db_real_free(current);
	}

	/* remove every data entry */
	remove_all(pf);

	/* free service name */
	if (pf->name)
		free(pf->name);

	/* free service struct */
	free(pf);
}


/* clear database */
void initng_active_db_free_all(void)
{
	active_db_h *current, *safe = NULL;

	while_active_db_safe(current, safe)
	{
		initng_active_db_free(current);
	}
}
