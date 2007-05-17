/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2006 Ismael Luceno <ismael.luceno@gmail.com>
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
#include "parse_args.h"


static void handle_it(char *str)
{
	int j;
	char *opt;
	char *val;

	val = strchr(opt, '=');

	if (!val)
		val = strchr(opt, ':');

	if (val)
	{
		val[0] = '\0';
		val++;
	}

	for (j = 0; opts[j].name != NULL; j++)
	{
		if (strcmp(opts[j].name, opt) == 0)
		{
			(opts[j].handle)(val);
			return;
		}
	}

	W_("Unknown option %s", opt);
}

void config_parse_args(char **argv)
{
	int i;
	char *opt;

	for (i = 0; argv[i] != NULL; i++)
	{
		opt = argv[i];

		/* don't parse options starting with an '+' */
		if (opt[0] == '+')
			continue;

		/*
		 * if arg is --verbose, skip the two -- here, and start checking.
		 * if arg is -verbose, skip and continue for loop.
		 * if arg is verbose, check it below
		 */

		if (opt[0] == '-')
		{
			if (opt[1] != '-')
				continue;

			opt += 2;
		}

		handle_it(opt);
	}
}

#define BUF_LEN 256

int config_parse_file(const char *file)
{
	FILE *f;
	char tmp[BUF_LEN + 1];

	if ((f = fopen(file, "r")) == NULL)
	{
		F_("Failed opening configuration file '%s'", file);
		return (-1);
	}
	
	while (fgets(tmp, BUF_LEN, f))
	{
		tmp[BUF_LEN] = '\0';
		handle_it(tmp);
	}

	fclose(f);
	return (0);
}
