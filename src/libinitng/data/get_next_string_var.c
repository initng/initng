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
#include "initng_global.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "initng_struct_data.h"
#include "initng_static_data_id.h"
#include "initng_toolbox.h"

#include "local.h"


const char *d_get_next_string_var(s_entry * type, const char *vn,
										 data_head * d, s_data ** cur)
{
	/* Temporary store string pointer here */
	const char *to_ret;

	/* find next var */
	s_data *current = d_get_next_var(type, vn, d, *cur);

	/* Get the string path out of it */
	to_ret = current ? current->t.s : NULL;

	/* update to next */
	*cur = current;

	/* return string */
	return (to_ret);
}
