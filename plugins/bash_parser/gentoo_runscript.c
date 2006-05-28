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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

extern char **environ;

int main(int argc, char *argv[])
{
	char path[1025], buf[10];
	char *servname;
	char *new_argv[24];
	int is_initng = 0, fd;

#ifdef DEBUG_EXTRA
	{										/* for debug */
		int i;

		printf("%i args: \n", argc);
		for (i = 0; argv[i]; i++)
			printf("argv[%i]: %s\n", i, argv[i]);
	}
#endif

	if((fd = open("/proc/1/cmdline",O_RDONLY)) >= 0) {
		if(read(fd, buf, 6) == 6) {
			if(strncmp(buf,"initng",6) == 0)
				is_initng = 1;
		}
		close(fd);
	}

	if(!is_initng) {
		execve("/sbin/runscript_orig", new_argv, environ);
		fprintf(stderr, "Couldn't exec /sbin/runscript_orig\n");
		exit(42);
	}

	/* check to no of arguments */
	if (argc != 3)
	{
		printf("Usage script <start> <stop> <setup>\n");
		exit(1);
	}

	

	/* replace startin '.' with full path */
	if (argv[1][0] == '.')
	{
		if (!getcwd(path, 1024))
		{
			printf("error exeuting /lib/ibin/runiscript.sh\n");
			exit(3);
		}
		strncat(path, &argv[1][1], 1024 - strlen(path));
	}
	else
	{
		strncpy(path, argv[1], 1024);
	}

	/* check that full path is provided */
	if (path[0] != '/')
	{
		printf("Please, always call script with full path.\n");
		exit(2);
	}

	/* cut service name from the last '/' found in service path */
	servname = strdup(strrchr(path, '/') + 1);
	if(strncmp(servname, "net.", 4) == 0)
		servname[3] = '/';
	/*printf("servname: %s\n", servname); */

	/* check if these are direct commands, then use ngc */
	if (strcmp(argv[2], "start") == 0)
	{
		new_argv[0] = strdup("/sbin/ngc");
		new_argv[1] = strdup("--start");
		new_argv[2] = strdup(servname);
		new_argv[3] = NULL;
		execve(new_argv[0], new_argv, environ);
		exit(30);
	}
	if (strcmp(argv[2], "stop") == 0)
	{
		new_argv[0] = strdup("/sbin/ngc");
		new_argv[1] = strdup("--stop");
		new_argv[2] = strdup(servname);
		new_argv[3] = NULL;
		execve(new_argv[0], new_argv, environ);
		exit(31);
	}
	if (strcmp(argv[2], "zap") == 0)
	{
		new_argv[0] = strdup("/sbin/ngc");
		new_argv[1] = strdup("--zap");
		new_argv[2] = strdup(servname);
		new_argv[3] = NULL;
		execve(new_argv[0], new_argv, environ);
		exit(32);
	}
	if (strcmp(argv[2], "status") == 0)
	{
		new_argv[0] = strdup("/sbin/ngc");
		new_argv[1] = strdup("--status");
		new_argv[2] = strdup(servname);
		new_argv[3] = NULL;
		execve(new_argv[0], new_argv, environ);
		exit(33);
	}
	/* end check */

	/* check if command is valid */
	if (strncmp(argv[2], "internal_", 9) != 0)
	{
		if(strcmp(argv[2]+9, "start") == 0 ||
		   strcmp(argv[2]+9, "stop") == 0 ||
		   strcmp(argv[2]+9, "daemon") == 0 ||
		   strcmp(argv[2]+9, "kill") == 0)
		{
			printf("Bad command\n");
			exit(3);
		}

		new_argv[0] = strdup("/sbin/ngc");
		new_argv[1] = strdup("--run");
		new_argv[2] = malloc(strlen(servname) + strlen(argv[2]) + 2);
		strcpy(new_argv[2], servname);
		new_argv[2][strlen(servname)] = ':';
		strcpy(new_argv[2] + strlen(servname) + 1, argv[2]);
	}

	/* set up the bash script to run */
	char script[1024];			/* plenty of space */
	
	strcpy(script, "#/bin/bash\n");
	if(strcmp(argv[2],"internal_setup") == 0) 
	{
		strcat(script,
		       "export PATH=/lib/ibin:$PATH\n");
		strcat(script, "fail() { echo \"$@\"; exit 1; }\n");
		strcat(script, "[[ ${RC_GOT_FUNCTIONS} != \"yes\" ]] && source /sbin/functions.sh\n");
		strcat(script, "[ -f /etc/conf.d/");
		strcat(script, servname);
		strcat(script, " ] && source /etc/conf.d/");
		strcat(script, servname);
		strcat(script, "\nsource ");
		strcat(script, path);
		strcat(script, " || fail source\n");
		strcat(script, "iregister service $SERVICE || fail iregister\n");
		strcat(script, "need() { iset $SERVICE need = \"$*\" || fail iset need; }\n");
		strcat(script, "use() { iset $SERVICE use = \"$*\"|| fail iset use; }\n");
		strcat(script, "depend\n");
		strcat(script, "iset $SERVICE exec start = \"$SERVICE_FILE internal_start\" || fail iset exec start\n");
		strcat(script, "for i in stop $opts; do if typeset -F \"${x}\" &>/dev/null ; then iset $SERVICE exec $i = \"$SERVICE_FILE internal_$i\" || fail iset exec $i; fi; done\n");
		strcat(script, "idone $SERVICE || exit $?\n");
	}
	else
	{
		strcat(script, &argv[2][9]);
		strcat(script, "() {\necho \"ERROR: ");
		strcat(script, servname);
		strcat(script, " command ");
		strcat(script, &argv[2][9]);
		strcat(script,
		       " not found.\"\nreturn 1\n}\nexport PATH=/lib/ibin:$PATH\n");
		strcat(script, "[[ ${RC_GOT_FUNCTIONS} != \"yes\" ]] && source /sbin/functions.sh\n");		
		strcat(script, "[ -f /etc/conf.d/");
		strcat(script, servname);
		strcat(script, " ] && source /etc/conf.d/");
		strcat(script, servname);
		strcat(script, "\nsource ");
		strcat(script, path);
		strcat(script, "\n");
		strcat(script, &argv[2][9]);
		strcat(script, "\nexit $?\n");
	}

	/* printf("strlen script: %i : \n\"%s\"\n", strlen(script), script); */

	/* set up new argv */
	new_argv[0] = strdup("/bin/bash");
	new_argv[1] = strdup("-c");
	new_argv[2] = script;
	new_argv[3] = NULL;

	/* set up the environments */
	setenv("SERVICE_FILE", strdup(path), 1);
	setenv("SERVICE", strdup(servname), 1);
	setenv("COMMAND", strdup(argv[2]), 1);
	setenv("THE_RIGHT_WAY", "TRUE", 1);


	/*printf("\nexecuting...\n\n"); */
	/* now call the bash script */
	execve(new_argv[0], new_argv, environ);

	/* Newer get here */
	printf("error exeuting /bin/bash.\n");
	exit(3);
}
