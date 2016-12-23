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
#define DECL_SYSCALL0(fn)                int syscall_##fn()
#define DECL_SYSCALL1(fn,p1)             int syscall_##fn(p1)
#define DECL_SYSCALL2(fn,p1,p2)          int syscall_##fn(p1,p2)
#define DECL_SYSCALL3(fn,p1,p2,p3)       int syscall_##fn(p1,p2,p3)
#define DECL_SYSCALL4(fn,p1,p2,p3,p4)    int syscall_##fn(p1,p2,p3,p4)
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) int syscall_##fn(p1,p2,p3,p4,p5)

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


DEFN_SYSCALL3(write, 4, int, char *, int);
DEFN_SYSCALL3(read, 3, int, char *, int);
DEFN_SYSCALL3(execve, 11, char *, char **, char **);
DEFN_SYSCALL3(open, 5, char *, int, int);
DEFN_SYSCALL3(lseek, 19, int, int, int);
DEFN_SYSCALL0(exit, 1);
DEFN_SYSCALL0(fork, 2);
DEFN_SYSCALL1(brk, 45, int);
DEFN_SYSCALL1(isatty, 200, int);
DEFN_SYSCALL2(fstat, 28, int, struct stat *);
DEFN_SYSCALL2(stat, 0x12, char *, struct stat *);
DEFN_SYSCALL2(kill, 37, int, int);
DEFN_SYSCALL2(link, 9, char*, char*);
DEFN_SYSCALL0(close, 6);
DEFN_SYSCALL0(getpid, 20);
DEFN_SYSCALL2(mkdir, 0x27, char *, mode_t);

__attribute__((noreturn))
void __stack_chk_fail(void)
{
    write(1, "Stack overflow!\n", 16);
    while(1){}
}

__attribute__((noreturn)) void _exit(int code)
{
    syscall_exit();
    while(1){}
};

int stat(const char *path, struct stat *buf)
{
    return syscall_stat(path, buf);
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    char *m = "sigprocmask!\n";
    write(1, m, strlen(m));
    return 0;
}

int getppid(void)
{
    return 45678;
}

int geteuid(void)
{
    return 123456;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    return 0;
}

int sigsuspend(const sigset_t *mask)
{
    char *m = "sigsuspend! infinity loop\n";
    write(1, m, strlen(m));
    while(1){}
    return -1;
}

int tcsetpgrp(int fd, pid_t pgrp)
{
    char *m = "tcsetpgrp!\n";
    write(1, m, strlen(m));
    return 0;
}

pid_t getpgrp(pid_t pid)
{
    char *m = "getpgrp!\n";
    write(1, m, strlen(m));
    return 0;
}

int pipe(int pipefd[2])
{
    char *m = "pipe!\n";
    write(1, m, strlen(m));
    return 0;
}

int getgid()
{
    char *m = "getgid!\n";
    write(1, m, strlen(m));
    return 0;
}

int getegid()
{
    char *m = "getegid!\n";
    write(1, m, strlen(m));
    return 0;
}

int chdir(const char *path)
{
    char *m = "chdir!\n";
    write(1, m, strlen(m));
    return 0;
}

int dup2(int oldfd, int newfd)
{
    char *m = "dup2!\n";
    write(1, m, strlen(m));
    return 0;
}

char *getcwd(char *buf, size_t size)
{
    buf[0] = "/";
    buf[1] = "\0";
    return buf;
}

int fcntl(int fildes, int cmd, ...)
{
    char *m = "fcntl!\n";
    write(1, m, strlen(m));
    return 0;
}

struct DIR *opendir(const char *name)
{
    char *m = "opendir!\n";
    write(1, m, strlen(m));
    return 0;
}

pid_t tcgetpgrp(int fd)
{
    char *m = "tcgetpgrp!\n";
    write(1, m, strlen(m));
    return 0;
}

int killpg(int pgrp, int sig)
{
    char *m = "killpg!\n";
    write(1, m, strlen(m));
    return 0;
}

pid_t wait3(int *status, int options, struct rusage *rusage)
{
    char *m = "wait3!\n";
    write(1, m, strlen(m));
    return 0;
}

int closedir(DIR *dirp)
{
    char *m = "closedir!\n";
    write(1, m, strlen(m));
    return 0;
}

struct dirent *readdir(DIR *dirp)
{
    char *m = "readdir!\n";
    write(1, m, strlen(m));
    return 0;
}

int setpgid()
{
    char *m = "setpgid!\n";
    write(1, m, strlen(m));
    return 0;
}

int getuid()
{
    char *m = "getuid!\n";
    write(1, m, strlen(m));
    return 0;
}

int getgroups(int size, gid_t list[])
{
    char *m = "getgroups!\n";
    write(1, m, strlen(m));
    return 0;
}

int lstat(const char *path, struct stat *buf)
{
    char *m = "lstat!\n";
    write(1, m, strlen(m));
    return 0;
}

mode_t umask(mode_t mask)
{
    char *m = "umask!\n";
    write(1, m, strlen(m));
    return 0;
}

int close(int file)
{
    return syscall_close(file);
}

extern char **environ; /* pointer to array of char * strings that define the current environment variables */

int execve(char *name, char **argv, char **env)
{
    return syscall_execve(name, argv, env);
}

int fork()
{
    return syscall_fork();
}

int fstat(int file, struct stat *st)
{
    return syscall_fstat(file, st);
}

int getpid()
{
    return syscall_getpid();
}

int isatty(int file)
{
    return syscall_isatty(file);
}

int kill(int pid, int sig)
{
    return syscall_kill(pid, sig);
}

int link(char *old, char *new)
{
    return syscall_link(old, new);
}

int lseek(int file, int ptr, int dir)
{
    return syscall_lseek(file, ptr, dir);
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

    return syscall_open(name, flags, O_WRONLY);
}

int read(int file, char *ptr, int len)
{
    return syscall_read(file, ptr, len);
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
    return syscall_mkdir(path, mode);
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

int stat(const char *file, struct stat *st);
clock_t times(struct tms *buf);
int unlink(char *name);
int wait(int *status);
int write(int file, char *ptr, int len)
{
    return syscall_write(file, ptr, len);
}

int gettimeofday(struct timeval *ptimeval, void *ptimezone) { return 0;};
