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

#include <termios.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/kd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/klog.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <initng.h>

#ifdef SELINUX
#include "selinux.h"
#endif

static void setup_console(const char *console)
{
	int fd;
	struct termios tty;

	fd = open(console, O_RDWR | O_NOCTTY);

	/* Try to open the console, but don't control it */
	if (fd > 0) {
		D_("Opened %s. Setting terminal options.\n", console);
	} else {
		D_("Failed to open %s. Setting options anyway.\n", console);
		fd = 0;
	}

	/* Accept signals from 'kbd' */
	/* Like Ctrl + Alt + Delete signal? */
	ioctl(fd, KDSIGACCEPT, SIGWINCH);

	/* TODO: document the following code */

	tcgetattr(fd, &tty);

	tty.c_cflag &= CBAUD | CBAUDEX | CSIZE | CSTOPB | PARENB | PARODD;
	tty.c_cflag |= HUPCL | CLOCAL | CREAD;

	tty.c_cc[VINTR] = 3;	/* ctrl('c') */
	tty.c_cc[VQUIT] = 28;	/* ctrl('\\') */
	tty.c_cc[VERASE] = 127;
	tty.c_cc[VKILL] = 24;	/* ctrl('x') */
	tty.c_cc[VEOF] = 4;	/* ctrl('d') */
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VSTART] = 17;	/* ctrl('q') */
	tty.c_cc[VSTOP] = 19;	/* ctrl('s') */
	tty.c_cc[VSUSP] = 26;	/* ctrl('z') */

	/* Set pre and post processing */
	tty.c_iflag = IGNPAR | ICRNL | IXON | IXANY;
	tty.c_oflag = OPOST | ONLCR;
	tty.c_lflag = ISIG | ICANON | ECHO | ECHOCTL | ECHOPRT | ECHOKE;

	/* Now set the terminal line.
	 * We don't care about non-transmitted output data and non-read input
	 * data.
	 */
	tcsetattr(fd, TCSANOW, &tty);
	tcflush(fd, TCIOFLUSH);
	close(fd);
}


/*
 * %%%%%%%%%%%%%%%%%%%%   main ()   %%%%%%%%%%%%%%%%%%%%
 */
#ifdef BUSYBOX
int init_main(int argc, char *argv[], char *env[])
#else
int main(int argc, char *argv[], char *env[])
#endif
{
	char **new_argv;
	const char *console = INITNG_CONSOLE;

#ifdef SELINUX
	setup_selinux();
#endif

	/* change dir to / */
	while (chdir("/") < 0) {
		F_("Can't chdir to /\n");
		initng_main_su_login();
	}

	/* set console loglevel */
	klogctl(8, NULL, 1);

	/* enable generation of core files */
	{
		struct rlimit c = { 1000000, 1000000 };
		setrlimit(RLIMIT_CORE, &c);
	}

	/* Disable Ctrl + Alt + Delete */
	reboot(RB_DISABLE_CAD);

	{
		int i;

 		new_argv = initng_toolbox_calloc(argc + 2, sizeof (char *));

		/* Copy argv into new_argv */
		for (i = 1; i < argc; i++) {
			new_argv[i] = argv[i];

			/* look for "console" option, if it's there, set
		 	* console to it, so we will open the desired console
		 	*/
			if (strncmp(argv[i], "console", 7) == 0 &&
			    (argv[i][7] == ':' || argv[i][7] == '='))
				console = &argv[i][8];
		}
		new_argv[0] = (char *) INITNG_CORE_BIN;
	}

	setup_console(console);

	if (getpid() != 1) {
		if (getuid() != 0) {
			W_("Initng is designed to run as root user, a lot of "
			   "functionality will not work correctly.\n");
		}
		new_argv[argc] = (char *) "--fake";
		argc++;
	}

	new_argv[argc] = NULL;

	execv(new_argv[0], new_argv);
	return 1;
}
