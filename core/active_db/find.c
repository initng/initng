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

/**
 * Search a service trought active_db by exact name.
 *
 * @param service name
 * @return service
 *
 * This is an exact search, "net/eth0" must be "net/eth0".
 */
active_db_h * initng_active_db_find_by_name(const char *service)
{
	active_db_h *current = NULL;

	assert(service);

	/* first, search for an exact match */
	while_active_db(current) {
		assert(current->name);
		/* check if this service name is like service */
		if (strcmp(current->name, service) == 0)
			return current;
	}

	/* did not found any */
	return NULL;
}

/**
 * Fuzzy search throught active_db.
 *
 * @param service name
 * @return service
 *
 * This function will do a search and find the best candidate for a gievn name
 * (e.g. a search for "eth" will give "net/eth0").
 */
active_db_h *initng_active_db_find_in_name(const char *service)
{
	active_db_h *current = NULL;

	assert(service);

	/* first search by name */
	if ((current = initng_active_db_find_by_name(service)))
		return (current);

	/* then search for a word match */
	current = NULL;
	while_active_db(current) {
		assert(current->name);
		if (initng_string_match_in_service(current->name, service))
			return (current);
	}

	/* did not find any */
	return (NULL);
}

/**
 * Searchs a service by PID.
 *
 * @param pid
 * @return service
 *
 * Returns a pointer to the active_db_h the process belongs to, and sets
 * process_type.
 */
active_db_h *initng_active_db_find_by_pid(pid_t pid)
{
	active_db_h *currentA = NULL;
	process_h *currentP = NULL;

	/* walk the active_db */
	while_active_db(currentA) {
		assert(currentA->name);
		currentP = NULL;
		while_processes(currentP, currentA) {
			if (currentP->pid == pid)
				return (currentA);
		}
	}

	/* bad luck */
	return (NULL);
}
