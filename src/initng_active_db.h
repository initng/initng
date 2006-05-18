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

#ifndef ACTIVE_DB_H
#define ACTIVE_DB_H

typedef struct active_type active_db_h;


#include <sys/types.h>
#include <unistd.h>
#include "initng_service_cache.h"

#include "initng_struct_data.h"
#include "initng_list.h"

#include "initng_service_types.h"
#include "initng_active_state.h"
#include "initng_process_db.h"

#define MAX_SUCCEEDED 30

/* the active service struct */
struct active_type
{
	/* identification */
	char *name;
	stype_h *type;

	/* service data */
	service_cache_h *from_service;

	/* current state */
	a_state_h *current_state;
	struct timeval time_current_state;	/* the time got current state */

	/* last    state */
	a_state_h *last_state;
	struct timeval time_last_state;	/* the time got last state */

	/* Rough  state */
	e_is last_rought_state;
	struct timeval last_rought_time;

	/* processes */
	process_h processes;

	/* list container, for data */
	data_head data;

	/* Alarm, the current state alarm is run when this time passes */
	time_t alarm;

	/* depend cache - Optimization to speed up UP_DEPS_CHECK */
	int depend_cache;

	/* the list */
	struct list_head list;
	struct list_head interrupt;
};

/* allocate */
active_db_h *initng_active_db_new(const char *name);

/* searching */
active_db_h *initng_active_db_find_by_exact_name(const char *service);
active_db_h *initng_active_db_find_by_name(const char *service);
active_db_h *initng_active_db_find_in_name(const char *service);
active_db_h *initng_active_db_find_by_pid(pid_t pid);
active_db_h *initng_active_db_find_by_service_h(service_cache_h * service);

/* mangling */
void initng_active_db_change_service_h(service_cache_h * from,
									   service_cache_h * to);
void initng_active_db_compensate_time(time_t skew);

/* reload service cache if not set */
int reload_service_cache(data_head * data);

/* the db */
#define initng_active_db_unregister(serv) list_del(&(serv)->list)
int initng_active_db_register(active_db_h * new_a);
int initng_active_db_count(a_state_h * state);
void initng_active_db_free(active_db_h * pf);
void initng_active_db_free_all(void);

/* utils */
int initng_active_db_percent_started(void);
int initng_active_db_percent_stopped(void);
void initng_active_db_clean_down(void);

/* walk the db */
#define while_active_db(current) list_for_each_entry_prev(current, &g.active_database.list, list)
#define while_active_db_safe(current, safe) list_for_each_entry_prev_safe(current, safe, &g.active_database.list, list)
#define while_active_db_interrupt(current) list_for_each_entry_prev(current, &g.active_database.interrupt, interrupt)
#define while_active_db_interrupt_safe(current, safe) list_for_each_entry_prev_safe(current, safe, &g.active_database.interrupt, interrupt)

#endif
