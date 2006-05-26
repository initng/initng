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

extern char ** environ;

int main(int argc, char *argv[])
{
	char path[1025];
	char *servname;
	char *new_argv[24];

#ifdef DEBUG_EXTRA
	{ /* for debug */
		int i;
		printf("%i args: \n", argc);
		for(i=0;argv[i];i++)
			printf("argv[%i]: %s\n", i, argv[i]);
	}
#endif

	/* check to no of arguments */
	if(argc != 3)
	{
		printf("Usage script <start> <stop> <setup>\n");
		exit(1);
	}

	/* replace startin '.' with full path */
	if(argv[1][0] == '.')
	{
	        if(!getcwd(path, 1024))
	        {
	              printf("error exeuting /lib/ibin/runiscript.sh\n");
		      exit(3);
      	        }
		strncat(path, &argv[1][1], 1024 - strlen(path));
	} else {
		strncpy(path, argv[1], 1024);
	}

	/* check that full path is provided */
	if(path[0] != '/')
	{
		printf("Please, always call script with full path.\n");
		exit(2);
	}

	/* cut service name from the last '/' found in service path */
	servname=strrchr(path, '/')+1;
	/*printf("servname: %s\n", servname);*/

	/* check if these are direct commands, then use ngc */
	if(strcmp(argv[2], "start")==0)
	{
		new_argv[0]=strdup("/sbin/ngc");
		new_argv[1]=strdup("--start");
		new_argv[1]=strdup(servname);
		new_argv[2]=NULL;
		execve(new_argv[0], new_argv, environ);
		exit(30);
	}
	if(strcmp(argv[2], "stop")==0)
	{
		new_argv[0]=strdup("/sbin/ngc");
		new_argv[1]=strdup("--stop");
		new_argv[1]=strdup(servname);
		new_argv[2]=NULL;
		execve(new_argv[0], new_argv, environ);
		exit(31);
	}
	if(strcmp(argv[2], "zap")==0)
	{
		new_argv[0]=strdup("/sbin/ngc");
		new_argv[1]=strdup("--zap");
		new_argv[1]=strdup(servname);
		new_argv[2]=NULL;
		execve(new_argv[0], new_argv, environ);
		exit(32);
	}
	if(strcmp(argv[2], "status")==0)
	{
		new_argv[0]=strdup("/sbin/ngc");
		new_argv[1]=strdup("--status");
		new_argv[1]=strdup(servname);
		new_argv[2]=NULL;
		execve(new_argv[0], new_argv, environ);
		exit(33);
	}
	/* end check */
	
	/* check if command is valid */
	if(strncmp(argv[2], "internal_", 9)!=0)
	{
		printf("Bad command\n");
		exit(3); 
	}
	
	/* set up the bash script to run */
	char script[1024];
	strcpy(script, "#/bin/bash\n");
	strcat(script, &argv[2][9]);
	strcat(script, "() {\n");
	strcat(script, "echo \"ERROR: ");
	strcat(script, servname);
	strcat(script, " command ");
	strcat(script, &argv[2][9]);
	strcat(script, " not found.\"\n");
	strcat(script, "return 0\n}\n");
	strcat(script, "export PATH=/lib/ibin:$PATH\n");
	strcat(script, "source ");
	strcat(script, path);
	strcat(script,"\n");
	strcat(script, &argv[2][9]);
	strcat(script,"\nexit $?\n");
	
	
	/* printf("strlen script: %i : \n\"%s\"\n", strlen(script), script); */
	
	/* set up new argv */
	new_argv[0]=strdup("/bin/bash");
	new_argv[1]=strdup("-c");
	new_argv[2]=script;
	new_argv[3]=NULL;

	/* set up the environments */
	setenv("SERVICE_FILE", strdup(path), 1);
	setenv("SERVICE", strdup(servname), 1);
	setenv("COMMAND", strdup(argv[2]), 1);
	setenv("THE_RIGHT_WAY", "TRUE", 1);
	
	
	/*printf("\nexecuting...\n\n");*/
	/* now call the bash script */
	execve(new_argv[0], new_argv, environ);
	
	/* Newer get here */
	printf("error exeuting /bin/bash.\n");
	exit(3);
}
