#include <signal.h>
#include <unistd.h>

extern int shutdown_flag;                 /* defined in main.c */
extern void writelog(const char *, ...);  /* defined in game.c */
extern void panic(const char *message);   /* defined in game.c */

static void graceful_exit(int);
static void hard_exit(int);

void set_signals(void) {
	struct sigaction ignore           = { .sa_handler = SIG_IGN,       .sa_flags = 0 };
	struct sigaction sa_graceful_exit = { .sa_handler = graceful_exit, .sa_flags = 0 };
	struct sigaction sa_hard_exit     = { .sa_handler = hard_exit,     .sa_flags = 0 };
	sigemptyset(&ignore.sa_mask);
	sigemptyset(&sa_hard_exit.sa_mask);
	sigemptyset(&sa_graceful_exit.sa_mask);

	/* we don't care about SIGPIPE, we notice it in select() and write() */
	sigaction(SIGPIPE, &ignore, 0);

	/* standard termination signals */
	sigaction(SIGINT,  &sa_graceful_exit, 0);
	sigaction(SIGTERM, &sa_graceful_exit, 0);

	/* catch these because we might as well */
	sigaction(SIGQUIT,   &sa_hard_exit, 0);
	sigaction(SIGILL,    &sa_hard_exit, 0);
	sigaction(SIGTRAP,   &sa_hard_exit, 0);
	sigaction(SIGIOT,    &sa_hard_exit, 0);
	sigaction(SIGFPE,    &sa_hard_exit, 0);
	sigaction(SIGBUS,    &sa_hard_exit, 0);
	sigaction(SIGSEGV,   &sa_hard_exit, 0);
	sigaction(SIGSYS,    &sa_hard_exit, 0);
	sigaction(SIGXCPU,   &sa_hard_exit, 0);
	sigaction(SIGXFSZ,   &sa_hard_exit, 0);
	sigaction(SIGVTALRM, &sa_hard_exit, 0);
	sigaction(SIGUSR1,   &sa_hard_exit, 0);
	sigaction(SIGUSR2,   &sa_hard_exit, 0);
}

static void hard_exit(int signum) {
	writelog("BAILOUT: caught signal %d\n", signum);
	panic("PANIC on spurious signal");
	_exit(7);
}

static void graceful_exit(int signum) {
	writelog("SHUTDOWN: on signal %d code %d\n", signum);
	shutdown_flag = 1;
}

