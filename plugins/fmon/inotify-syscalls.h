#ifndef _LINUX_INOTIFY_SYSCALLS_H
#define _LINUX_INOTIFY_SYSCALLS_H

#include <sys/syscall.h>

#if defined(__i386__)
# define __NR_inotify_init	291
# define __NR_inotify_add_watch	292
# define __NR_inotify_rm_watch	293
#elif defined(__x86_64__)
# define __NR_inotify_init	253
# define __NR_inotify_add_watch	254
# define __NR_inotify_rm_watch	255
#elif defined(__powerpc__)
# define __NR_inotify_init      275
# define __NR_inotify_add_watch 276
# define __NR_inotify_rm_watch  277
#elif defined (__ia64__)
# define __NR_inotify_init	1277
# define __NR_inotify_add_watch	1278
# define __NR_inotify_rm_watch	1279
#else
# error "Unsupported architecture!"
#endif

static inline int inotify_init(void)
{
	return syscall(__NR_inotify_init);
}

static inline int inotify_add_watch(int fd, const char *name, __u32 mask)
{
	return syscall(__NR_inotify_add_watch, fd, name, mask);
}

static inline int inotify_rm_watch(int fd, __u32 wd)
{
	return syscall(__NR_inotify_rm_watch, fd, wd);
}

#endif /* _LINUX_INOTIFY_SYSCALLS_H */
