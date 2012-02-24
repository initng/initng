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
#include <sys/mount.h>

#include <initng.h>
#include <initng-paths.h>

#ifdef SELINUX
#include "selinux.h"
#endif

#ifndef CBAUD // FIXME
#define CBAUD   0
#define CBAUDEX 0
#define ECHOCTL 0
#define ECHOKE  0
#define ECHOPRT 0
#endif

static inline void setup_console(const char *console)
{
	int fd;
	struct termios tty;

	fd = open(console, O_RDWR | O_NOCTTY);

	/* Try to open the console, but don't control it */
	if (fd > 0)
		fd = 0;

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


#define SULOGIN_ON_FAIL(x, errmsg) { \
		while ((x) < 0) { \
			perror(errmsg); \
			initng_main_su_login(); \
		} \
	}

/*
 * %%%%%%%%%%%%%%%%%%%%   main ()   %%%%%%%%%%%%%%%%%%%%
 */
int main(int argc, char *argv[], char *env[])
{
#ifdef SELINUX
	setup_selinux();
#endif

	SULOGIN_ON_FAIL(chdir("/"), "can't chdir to /");

	/* FIXME: linux-only code! */
	SULOGIN_ON_FAIL(mount("none", "/run", "tmpfs", 0, ""),
			"unable to mount /run");

	SULOGIN_ON_FAIL(mkdir("/run/initng", 0700),
			"can't make /run/initng");

	/* set console loglevel */
	klogctl(8, NULL, 1);

	/* enable generation of core files */
	{
		struct rlimit c = { 1000000, 1000000 };
		setrlimit(RLIMIT_CORE, &c);
	}

	/* Disable Ctrl + Alt + Delete */
	reboot(RB_DISABLE_CAD);

	const char *console = INITNG_CONSOLE;

	/* Look for "console" option, so we open the desired device. */
	for (int i = argc; i > 0; i--) {
		if (strncmp(argv[i], "console", 7) == 0
		    && (argv[i][7] == ':' || argv[i][7] == '=')) {
			console = &argv[i][8];
			break;
		}
	}

	setup_console(console);

	argv[0] = (char *) INITNG_CORE_BIN;
	execv(argv[0], argv);
	return 1;
}
