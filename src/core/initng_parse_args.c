#include <string.h>

#include "initng_parse_args.h"
#include "initng_toolbox.h"
#include "initng_global.h"

typedef struct {
	const char *name;
	void (*handle)(char *val);
} opts_t;


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

static void opt_fake(char *val)
{
	g.i_am = I_AM_FAKE_INIT;
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



opts_t opts[] = {
	{ "console",		&opt_console		},
	{ "runlevel",		&opt_runlevel		},
	{ "fake",		&opt_fake		},
	{ "hot_reload",		&opt_hot_reload		},
	{ "no_circular",	&opt_no_circular	},
#ifdef DEBUG
	{ "verbose_add",	&opt_verbose_add	},
	{ "verbose",		&opt_verbose		},
#endif
	{ NULL,			NULL			}
};


void initng_parse_args(char **argv)
{
	int i, j;
	char *opt;
	char *val;

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
			if (opt[1] == '-')
				opt += 2;
			else /* but a single is not ! */
				continue;
		}

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
				break;
			}
		}
	}
}
