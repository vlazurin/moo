#ifndef H_SIGNAL
#define H_SIGNAL

#define SIGKILL 9
#define SIGCHLD 17
#define SIGSTOP 19

typedef void (*sig_func_ptr)(int);

#define SIG_DFL ((sig_func_ptr)0)	/* Default action */
#define SIG_IGN ((sig_func_ptr)1)	/* Ignore action */
#define SIG_ERR ((sig_func_ptr)-1)	/* Error return */

#define SA_NOCLDSTOP 1
#define SA_SIGINFO   2

typedef unsigned long sigset_t;

struct sigaction {
  int         sa_flags;
  sigset_t    sa_mask;
  sig_func_ptr sa_handler;
};

#endif
