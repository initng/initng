/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2006 Ismael Luceno <ismael.luceno@gmail.com>
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

#ifndef INITNG_EVENT_TYPES_H
#define INITNG_EVENT_TYPES_H

#include <initng/list.h>
#include <initng/module.h>

typedef struct s_event_type_s {
	const char *name;
	const char *description;

	s_call hooks;

	int name_len;
	list_t list;
} s_event_type;

void initng_event_type_register(s_event_type *ent);
void initng_event_type_unregister(s_event_type *ent);
void initng_event_type_unregister_all(void);
s_event_type *initng_event_type_find(const char *string);

#define while_event_types(current) \
	initng_list_foreach_rev(current, &g.event_db.list, list)

#define while_event_types_safe(current, safe) \
	initng_list_foreach_rev_safe(current, safe, &g.event_db.list, list)

#endif /* INITNG_EVENT_TYPES_H */
