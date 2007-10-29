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

#ifndef INITNG_SYSLOG_H
#define INITNG_SYSLOG_H
#include <sys/types.h>
#include <initng.h>

typedef struct {
	int prio;
	char *owner;
	char *buffert;
	list_t list;
} log_ent;

#define while_log_list_safe(current, safe) \
	initng_list_foreach_rev_safe(current, safe, &log_list.list, list)

#endif /* ! INITNG_SYSLOG_H */
