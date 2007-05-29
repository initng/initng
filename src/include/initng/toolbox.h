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

#ifndef INITNG_TOOLBOX_H
#define INITNG_TOOLBOX_H

#include <initng/misc.h>

/*
   #undef initng_calloc
   #define initng_calloc(nemb, size) \
   initng_calloc2(nemb, size, __func__, __LINE__)
   void *initng_calloc2(size_t nmemb, size_t size, const char *func, int line);
*/
void *initng_toolbox_calloc(size_t nmemb, size_t size);

#undef initng_toolbox_realloc
#define initng_toolbox_realloc(ptr, size) \
    initng_toolbox_realloc2(ptr, size, (const char*)__func__, __LINE__)
void *initng_toolbox_realloc2(void *ptr, size_t size, const char *func, int line);

#undef initng_toolbox_strdup
#define initng_toolbox_strdup(s) \
	initng_toolbox_strdup2(s, (const char*)__func__, __LINE__)
char *initng_toolbox_strdup2(const char *s, const char *func, int line);

#undef initng_toolbox_strndup
#define initng_toolbox_strndup(s, n) \
	initng_toolbox_strndup2(s, n, (const char*)__func__, __LINE__)
char *initng_toolbox_strndup2(const char *s, size_t n, const char *func, int line);
int initng_toolbox_set_proc_title(const char *fmt, ...);

#define MS_DIFF(A, B) (int)((int)(((A).tv_sec * 1000) + ((A).tv_usec / 1000)) - (int)(((B).tv_sec * 1000) + ((B).tv_usec / 1000)))
#endif /* INITNG_TOOLBOX_H */
