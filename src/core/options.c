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

#include <string.h>
#include <initng.h>

#include "options.h"

#define OPT_HELP	0
#define OPT_CONSOLE	1
#define OPT_RUNLEVEL	2
#define OPT_HOT_RELOAD	4
#define OPT_NO_CIRCULAR	5
#define OPT_VERBOSE_ADD	6
#define OPT_VERBOSE	7
#define OPT_VERSION	8

opt_t opts[] = {
	{OPT_CONSOLE, "console",
	 "Specify what dev to use as console."}
	,
	{OPT_RUNLEVEL, "runlevel",
	 "Specify default runlevel."}
	,
	{OPT_HOT_RELOAD, "hot_reload",
	 NULL}
	,
	{OPT_NO_CIRCULAR, "no_circular",
	 "Make extra checkings for cirular depencenis "
	 "in service, takes some extra cpu but might "
	 "work when initng won't."}
	,
#ifdef DEBUG
	{OPT_VERBOSE_ADD, "verbose_add",
	 "Add one function to the list of "
	 "debug-arguments that will be printed."}
	,
	{OPT_VERBOSE, "verbose",
	 "Make initng be very verbose about " "what's happening."}
	,
#endif
	{OPT_HELP, "help",
	 "Show this help list."}
	,
	{OPT_VERSION, "version",
	 "Show the version information."}
	,
	{0, NULL, NULL}
};

static void handle_it(char *str)
{
	char *val;
	int i;

	switch (initng_config_opt_get(opts, &val, str)) {
	case OPT_CONSOLE:
		if (val)
			g.dev_console = initng_toolbox_strdup(val);
		break;

	case OPT_RUNLEVEL:
		if (val)
			g.runlevel = initng_toolbox_strdup(val);
		break;

	case OPT_NO_CIRCULAR:
		g.no_circular = TRUE;
		break;

	case OPT_HOT_RELOAD:
		D_(" Will start after a hot reload ...\n");
		g.hot_reload = TRUE;
		break;

#ifdef DEBUG
	case OPT_VERBOSE:
		g.verbose = TRUE;
		break;

	case OPT_VERBOSE_ADD:
		if (val)
			initng_error_verbose_add(val);
		break;
#endif

	case OPT_HELP:
		printf("Options are given to initng by linux "
		       "bootloader, you can use option=value, or "
		       "option:value to set an option.\n\n"
		       "Possible options:\n");

		for (i = 0; opts[i].name; i++) {
			if (opts[i].desc && (!val ||
					     strcmp(opts[i].name, val) == 0)) {
				printf(" %16s: %s\n", opts[i].name,
				       opts[i].desc);
			}
		}

		printf("\n\n");
		_exit(0);

	case OPT_VERSION:
		printf(INITNG_VERSION " (API %i)\n"
		       INITNG_COPYRIGHT, API_VERSION);
		_exit(0);

	default:
		break;
	}
}

void options_parse_args(char **argv)
{
	int i;
	char *opt;

	for (i = 0; argv[i] != NULL; i++) {
		opt = argv[i];

		/* don't parse options starting with an '+' */
		if (opt[0] == '+')
			continue;

		/*
		 * if arg is --verbose, skip the two -- here, and start checking.
		 * if arg is -verbose, skip and continue for loop.
		 * if arg is verbose, check it below
		 */

		if (opt[0] == '-') {
			if (opt[1] != '-')
				continue;

			opt += 2;
		}

		handle_it(opt);
	}
}

#define BUF_LEN 256

int options_parse_file(const char *file)
{
	FILE *f;
	char tmp[BUF_LEN + 1];

	if ((f = fopen(file, "r")) == NULL) {
		F_("Failed opening configuration file '%s'", file);
		return -1;
	}

	while (fgets(tmp, BUF_LEN, f)) {
		tmp[BUF_LEN] = '\0';
		handle_it(tmp);
	}

	fclose(f);
	return 0;
}
