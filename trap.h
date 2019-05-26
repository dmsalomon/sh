
#ifndef TRAP_H
#define TRAP_H

#include <signal.h>

static inline void sigclearmask(void) {
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, 0);
}

void signal_init(void);
void onsig(int);

#endif
