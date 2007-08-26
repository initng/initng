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

#ifndef INITNG_SERVICE_TYPES_H
#define INITNG_SERVICE_TYPES_H

#include <initng/list.h>
#include <initng/active_db.h>

/* service types struct */
typedef struct {
	/* Name of service type */
	const char *name;

	/* The description */
	const char *description;

	/* IF the services is shown by ngc -s */
	int hidden;

	/* Called to start service */
	int (*start) (active_db_h *service);

	/* Called to stop service */
	int (*stop) (active_db_h *service);

	/* Called to restart service */
	int (*restart) (active_db_h *service);

	/* length of name_len */
	int name_len;

	/* list of stypes */
	struct list_head list;
} stype_h;

/* make sure name_len is set */
#define initng_service_type_register(st) { if((st)->name) { (st)->name_len=strlen((st)->name); } else { (st)->name_len=0; } list_add(&(st)->list, &g.stypes.list); }

#define initng_service_type_unregister(st) list_del(&(st)->list)
/* service_db walker */
#define while_service_types(current) list_for_each_entry_prev(current, &g.stypes.list, list)
#define while_service_types_safe(current, safe) list_for_each_entry_prev_safe(current, safe, &g.stypes.list, list)

stype_h *initng_service_type_get_by_name(const char *name);

#endif /* INITNG_SERVICE_TYPES_H */
