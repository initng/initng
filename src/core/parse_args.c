#include <string.h>

#include "parse_args.h"

#include <initng.h>

typedef struct {
	const char *name;
	void (*handle)(char *val);
	const char *desc;
} opts_t;


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
		"Specify what dev to use as console"	},
	{ "help",		&list_options,
		"Show this help list"			},
	{ "runlevel",		&opt_runlevel,
		"Specify default runlevel"		},
	{ "i_am_init",		&opt_i_am_init,
		"Start initng in real init mode, "
		"instead of fake mode"			},
	{ "hot_reload",		&opt_hot_reload, NULL	},
	{ "no_circular",	&opt_no_circular,
		"Make extra checkings for cirular "
		"depencenis in service, takes some "
		"extra cpu but might work when initng "
		"wont."					},
#ifdef DEBUG
	{ "verbose_add",	&opt_verbose_add,
		"Add one function to the list of "
		"debug-arguments that will be printed"	},
	{ "verbose",		&opt_verbose,
		"Make initng be verry verbose about "
		"whats happening."			},
#endif
	{ NULL,			NULL, NULL		}
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
	int i, j;
	printf("options are given to initng by linux bootloader, you can use option=value, or option:value to set an option.\n\n");
	printf("Possible options: \n");
	for(i = 0; opts[i].name; i++)
	{
		if(opts[i].desc)
		{
			printf(" %s:", opts[i].name);
			for(j = strlen(opts[i].name); j < 16; j++)
				printf(" ");
			printf("%s\n", opts[i].desc);
		}
	}
	printf("\n\n");
	_exit(0);
}



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
