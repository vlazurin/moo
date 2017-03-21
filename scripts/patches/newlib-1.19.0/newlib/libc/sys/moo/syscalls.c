/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define F_DUPFD_CLOEXEC 14

typedef struct sigset
{

}sigset_t;

typedef struct dirent {
	uint32_t d_ino;
	char d_name[256];
} dirent;

typedef struct DIR {
	int fd;
	int cur_entry;
    struct dirent ent;
} DIR;

#define DEFN_SYSCALL0(fn, num) \
	int syscall_##fn() { \
		int a; __asm__ __volatile__("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

#define DEFN_SYSCALL1(fn, num, P1) \
	int syscall_##fn(P1 p1) { \
		int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx" \
				: "=a" (__res) \
				: "0" (num), "r" ((int)(p1))); \
		return __res; \
	}

#define DEFN_SYSCALL2(fn, num, P1, P2) \
	int syscall_##fn(P1 p1, P2 p2) { \
		int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx" \
				: "=a" (__res) \
				: "0" (num), "r" ((int)(p1)), "c"((int)(p2))); \
		return __res; \
	}

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
	int syscall_##fn(P1 p1, P2 p2, P3 p3) { \
		int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx" \
				: "=a" (__res) \
				: "0" (num), "r" ((int)(p1)), "c"((int)(p2)), "d"((int)(p3))); \
		return __res; \
	}


#define SYSCALL_EXIT 1
#define SYSCALL_FORK 2
#define SYSCALL_READ 3
#define SYSCALL_WRITE 4
#define SYSCALL_OPEN 5
#define SYSCALL_CLOSE 6
#define SYSCALL_BRK 8
#define SYSCALL_STAT 9
#define SYSCALL_GETPID 10
#define SYSCALL_EXECVE 11
#define SYSCALL_CHDIR 12
#define SYSCALL_GETGID 13
#define SYSCALL_TCSETPGRP 14
#define SYSCALL_LSEEK 15
#define SYSCALL_TCGETPGRP 16
#define SYSCALL_SETPGRP 17
#define SYSCALL_FCNTL 18
#define SYSCALL_GETPGRP 19
#define SYSCALL_SIGACTION 20
#define SYSCALL_MKDIR 21
#define SYSCALL_WAITPID 22
#define SYSCALL_SIGSUSPEND 23
#define SYSCALL_DEBUG 24
#define SYSCALL_GETPPID 25
#define SYSCALL_GETEGID 26
#define SYSCALL_GETEUID 27
#define SYSCALL_GETUID 28
#define SYSCALL_OPENDIR 29
#define SYSCALL_READDIR 30
#define SYSCALL_CLOSEDIR 31

DEFN_SYSCALL0(fork, SYSCALL_FORK);
DEFN_SYSCALL3(write, SYSCALL_WRITE, int, char *, int);
DEFN_SYSCALL3(read, SYSCALL_READ, int, char *, int);
DEFN_SYSCALL3(execve, SYSCALL_EXECVE, char *, char **, char **);
DEFN_SYSCALL3(open, SYSCALL_OPEN, char *, int, int);
DEFN_SYSCALL3(lseek, SYSCALL_LSEEK, int, int, int);
DEFN_SYSCALL0(exit, SYSCALL_EXIT);
DEFN_SYSCALL1(brk, SYSCALL_BRK, int);
DEFN_SYSCALL1(chdir, SYSCALL_CHDIR, char*);
DEFN_SYSCALL1(isatty, 200, int);
DEFN_SYSCALL0(getppid, SYSCALL_GETPPID);
DEFN_SYSCALL0(geteuid, SYSCALL_GETEUID);
DEFN_SYSCALL0(getuid, SYSCALL_GETUID);
DEFN_SYSCALL0(getpgrp, SYSCALL_GETPGRP);
DEFN_SYSCALL0(getgid, SYSCALL_GETGID);
DEFN_SYSCALL0(getegid, SYSCALL_GETEGID);
DEFN_SYSCALL2(dup2, 206, int, int);
DEFN_SYSCALL2(getcwd, 207, char*, size_t);
DEFN_SYSCALL1(pipe, 208, int*);
DEFN_SYSCALL2(debug, SYSCALL_DEBUG, char *, int);
DEFN_SYSCALL2(fstat, 280, int, struct stat *);
DEFN_SYSCALL2(tcsetpgrp, SYSCALL_TCSETPGRP, int, pid_t);
DEFN_SYSCALL1(tcgetpgrp, SYSCALL_TCGETPGRP, int);
DEFN_SYSCALL2(stat, SYSCALL_STAT, char *, struct stat *);
DEFN_SYSCALL2(kill, 37, int, int);
DEFN_SYSCALL2(link, 9, char*, char*);
DEFN_SYSCALL2(setpgrp, SYSCALL_SETPGRP, pid_t, pid_t);
DEFN_SYSCALL1(close, SYSCALL_CLOSE, int);
DEFN_SYSCALL0(getpid, SYSCALL_GETPID);
DEFN_SYSCALL2(mkdir, SYSCALL_MKDIR, char *, mode_t);
DEFN_SYSCALL3(sigprocmask, 0x7e, int, sigset_t*, sigset_t*);
DEFN_SYSCALL3(waitpid, SYSCALL_WAITPID, int, int*, int);
DEFN_SYSCALL1(sigsuspend, SYSCALL_SIGSUSPEND, sigset_t*);
DEFN_SYSCALL3(sigaction, SYSCALL_SIGACTION, int, struct sigaction*, struct sigaction*);
DEFN_SYSCALL3(fcntl, SYSCALL_FCNTL, int, int, int);
DEFN_SYSCALL1(opedir, SYSCALL_OPENDIR, char*);
DEFN_SYSCALL1(readdir, SYSCALL_READDIR, struct DIR*);
DEFN_SYSCALL1(closedir, SYSCALL_CLOSEDIR, struct DIR*);

__attribute__((noreturn)) void __stack_chk_fail(void)
{
    write(1, "Stack overflow!\n", 16);
    while(1){}
}

__attribute__((noreturn)) void _exit(int code)
{
    syscall_exit();
};

int stat(const char *path, struct stat *buf)
{
    int i = syscall_stat(path, buf);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    int i = syscall_sigprocmask(how, set, oldset);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int getppid(void)
{
    return syscall_getppid();
}

int geteuid(void)
{
    return syscall_geteuid();
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    int i = syscall_sigaction(signum, act, oldact);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

// always returns error (normally EINTR)
int sigsuspend(const sigset_t *mask)
{
    errno = syscall_sigsuspend(mask);
    return -1;
}

int tcsetpgrp(int fd, pid_t pgrp)
{
    int i = syscall_tcsetpgrp(fd, pgrp);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

pid_t getpgrp()
{
    int i = syscall_getpgrp();
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int pipe(int pipefd[2])
{
    int i = syscall_pipe((int*)pipefd);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int getgid()
{
    return syscall_getgid();
}

int getegid()
{
    return syscall_getegid();
}

int chdir(const char *path)
{
    int i = syscall_chdir(path);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int dup2(int oldfd, int newfd)
{
    int i = syscall_dup2(oldfd, newfd);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

char *getcwd(char *buf, size_t size)
{
    int i = syscall_getcwd(buf, size);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int fcntl(int fd, int cmd, ...)
{
    int arg = 0;
    if (cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC || cmd == F_SETFD) {
        va_list args;
        va_start(args, cmd);
        arg = va_arg(args, int);
        va_end(args);
    }
    int i = syscall_fcntl(fd, cmd, arg);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

struct DIR *opendir(const char *name)
{
    int i = syscall_opedir(name);
    if (i < 0) {
        errno = i;
        return NULL;
    }
    struct DIR *d = malloc(sizeof(struct DIR));
    d->cur_entry = 0;
    d->fd = i;

    return d;
}

int closedir(DIR *dirp)
{
    int i = syscall_closedir(dirp);
    if (i < 0) {
        errno = i;
        return -1;
    }
}

struct dirent *readdir(DIR *dirp)
{
    int i = syscall_readdir(dirp);
    if (i < 0) {
        errno = i;
        return NULL;
    }
    return &dirp->ent;
}

pid_t tcgetpgrp(int fd)
{
    int i = syscall_tcgetpgrp(fd);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int killpg(int pgrp, int sig)
{
    syscall_debug("killpg isn't implemented\n", 26);
    return 0;
}

pid_t wait3(int *status, int options, struct rusage *rusage)
{
    int i = syscall_waitpid(-1, status, options);
	if (i < 0) {
		errno = -i;
		return -1;
	}
	return i;
}

int setpgid(pid_t pid, pid_t pgid)
{
    return syscall_setpgrp(pid, pgid);
}

int getuid()
{
    return syscall_getuid();
}

int getgroups(int size, gid_t list[])
{
    syscall_debug("getgroups isn't implemented\n", 29);
    return -1;
}

int lstat(const char *path, struct stat *buf)
{
    syscall_debug("lstat isn't implemented\n", 25);
    return -1;
}

mode_t umask(mode_t mask)
{
    syscall_debug("umask isn't implemented\n", 25);
    return -1;
}

int close(int file)
{
    int i = syscall_close(file);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

extern char **environ; /* pointer to array of char * strings that define the current environment variables */

int execve(const char *name, char **argv, char **envp)
{
	int i = syscall_execve((char*)name, argv, envp);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int fork()
{
    int i = syscall_fork();
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int fstat(int file, struct stat *st)
{
    int i = syscall_fstat(file, st);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int getpid()
{
    return syscall_getpid();
}

int isatty(int file)
{
    return 1;
    syscall_debug("isatty isn't implemented\n", 26);
    return 0;
}

int kill(int pid, int sig)
{
    syscall_debug("kill isn't implemented\n", 24);
    return -1;
}

int link(char *old, char *new)
{
    syscall_debug("link isn't implemented\n", 24);
    return -1;
}

int lseek(int file, int ptr, int dir)
{
    int i = syscall_lseek(file, ptr, dir);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int open(const char *name, int flags, ...)
{
    int mode = 0;
    if (flags & O_CREAT)
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    int i = syscall_open(name, flags, mode);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int read(int file, char *ptr, int len)
{
    int i = syscall_read(file, ptr, len);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

static caddr_t current_break = 0;

static int brk(void *end_data_segment)
{
     char *new_brk;

     new_brk = syscall_brk(end_data_segment);
     if (new_brk != end_data_segment) return -1;
     current_break = new_brk;
     return 0;
}

int mkdir(const char *path, mode_t mode)
{
    syscall_mkdir(path, mode);
}

caddr_t sbrk(int incr)
{
    char *old_brk, *new_brk;
    if (!current_break) current_break = syscall_brk(NULL);

    new_brk = syscall_brk(current_break+incr);
    if (new_brk != current_break+incr) return (void *) -1;
    old_brk = current_break;
    current_break = new_brk;
    return old_brk;
}

clock_t times(struct tms *buf)
{
    syscall_debug("times isn't implemented\n", 25);
    return -1;
}

int unlink(char *name)
{
    syscall_debug("unlink isn't implemented\n", 26);
    return -1;
}

int wait(int *status)
{
    syscall_debug("wait isn't implemented\n", 24);
    return -1;
}

int write(int file, char *ptr, int len)
{
    int i = syscall_write(file, ptr, len);
    if (i < 0) {
        errno = i;
        return -1;
    }
    return i;
}

int gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
    syscall_debug("gettimeofday isn't implemented\n", 32);
    return -1;
}
