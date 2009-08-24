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

#ifndef INITNG_TOOLBOX_H
#define INITNG_TOOLBOX_H

#include <initng/misc.h>

void *initng_toolbox_calloc(size_t nmemb, size_t size);

void *initng_toolbox_realloc(void *ptr, size_t size);

char *initng_toolbox_strdup(const char *s);

char *initng_toolbox_strndup(const char *s, size_t n);
int initng_toolbox_set_proc_title(const char *fmt, ...);

#define MS_DIFF(A, B) (int)((int)(((A).tv_sec * 1000) + ((A).tv_usec / 1000)) - (int)(((B).tv_sec * 1000) + ((B).tv_usec / 1000)))
#endif /* INITNG_TOOLBOX_H */
