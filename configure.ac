#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([InitNG], [0.7-GIT], [http://initng.sourceforge.net/trac])

AC_CONFIG_FILES([config.jam])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_SED

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h shadow.h stddef.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/time.h syslog.h termios.h unistd.h utmp.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_STDBOOL
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm dup2 getcwd getspnam gettimeofday memmove memset mkdir mkfifo putenv select setenv socket strchr strcspn strdup strerror strncasecmp strrchr strstr strtol])

# Checks for addional, third-party libraries
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([DBUS], [dbus-1])

# Strip -I from include flags for Jam.
DBUS_INCLUDES=`echo $DBUS_CFLAGS | sed -e 's,-\[lL\]\[^ \]*,,g' -e 's,-I,,g'`
AC_SUBST([DBUS_INCLUDES])

AC_OUTPUT_INSTALLDIRS
AC_OUTPUT
