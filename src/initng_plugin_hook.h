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

#ifndef INITNG_PLUGIN_HOOK
#define INITNG_PLUGIN_HOOK

#include "initng_plugin.h"

void initng_plugin_hook_unregister_real(const char *from_file,
										const char *func, int line,
										s_call * t, void *hook);
int initng_plugin_hook_register_real(const char *from_file, s_call * t,
									 int order, void *hook);
#define initng_plugin_hook_register(t,o,h) \
    initng_plugin_hook_register_real(__FILE__, t, o, h)
#define initng_plugin_hook_unregister(t,h) \
    initng_plugin_hook_unregister_real(__FILE__, (const char*)__PRETTY_FUNCTION__, __LINE__, t, h)

#endif
