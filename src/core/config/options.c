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


#include <string.h>
#include <initng.h>

#include "options.h"

#ifdef DEBUG
static void opt_verbose(char *val);
static void opt_verbose_add(char *val);
#endif

static void opt_no_circular(char *val);
static void opt_hot_reload(char *val);
static void opt_i_am_init(char *val);
static void opt_runlevel(char *val);
static void opt_console(char *val);
static void list_options(char *val);


opts_t opts[] = {
	{ "console",		&opt_console,
		"Specify what dev to use as console."		},
	{ "help",		&list_options,
		"Show this help list."				},
	{ "runlevel",		&opt_runlevel,
		"Specify default runlevel."			},
	{ "i_am_init",		&opt_i_am_init,
		"Start initng in real init mode, "
		"instead of fake mode."				},
	{ "hot_reload",		&opt_hot_reload, NULL		},
	{ "no_circular",	&opt_no_circular,
		"Make extra checkings for cirular depencenis "
		"in service, takes some extra cpu but might "
		"work when initng won't."			},
#ifdef DEBUG
	{ "verbose_add",	&opt_verbose_add,
		"Add one function to the list of "
		"debug-arguments that will be printed."		},
	{ "verbose",		&opt_verbose,
		"Make initng be very verbose about "
		"what's happening."				},
#endif
	{ NULL,			NULL,		NULL		}
};


#ifdef DEBUG
static void opt_verbose(char *val)
{
	g.verbose = TRUE;
}

static void opt_verbose_add(char *val)
{
	if (val)
		initng_error_verbose_add(val);
}
#endif

static void opt_no_circular(char *val)
{
	g.no_circular = TRUE;
}

static void opt_hot_reload(char *val)
{
	D_(" Will start after a hot reload ...\n");
	g.hot_reload = TRUE;
}

static void opt_i_am_init(char *val)
{
	g.i_am = I_AM_INIT;
}

static void opt_runlevel(char *val)
{
	if (val)
		g.runlevel = i_strdup(val);
}

static void opt_console(char *val)
{
	if (val)
		g.dev_console = i_strdup(val);
}

static void list_options(char *val)
{
	int i;
	printf("Options are given to initng by linux bootloader, you can use "
	       "option=value, or option:value to set an option.\n\n");
	printf("Possible options: \n");

	for (i = 0; opts[i].name; i++)
	{
		if (opts[i].desc && (!val || strcmp(opts[i].name, val) == 0))
			printf(" %16s: %s\n", opts[i].name, opts[i].desc);
	}
	printf("\n\n");
	_exit(0);
}
