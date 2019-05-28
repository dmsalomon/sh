
#ifndef MEM_H
#define MEM_H

#define ALIGNMENT (sizeof(union {int i; char *cp; double d; }) - 1)
#define MEMALIGN(n) (((n) + (ALIGNMENT)) & ~(ALIGNMENT))

#include <stdlib.h>

struct stackmark {
	struct stack_block *stackp;
	char *stacknext;
	size_t stacknleft;
};

void pushstackmark(struct stackmark *);
void popstackmark(struct stackmark *);

extern char *stacknext;
extern size_t stacknleft;
extern char *sstrend;

void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *);
void *stalloc(size_t);
void stfree(void *);

char *growstackstr();
char *growstackto(size_t);
char *stputs(const char *, char *);
char *sstrdup(const char *);

static inline char *_STPUTC(int c, char *p)
{
	if (p == sstrend)
		p = growstackstr();
	*p++ = c;
	return p;
}

#define STARTSTACKSTR(p) ((p) = stacknext)
#define STPUTC(c, p) ((p) = _STPUTC((c), (p)))
#define STUNPUTC(p) (--(p))
#define STTOPC(p) (p[-1])
#define STACKSTRNUL(p)	((p) == sstrend? (p = growstackstr(), *p = '\0') : (*p = '\0'))

static inline char *ststrsave(char *p)
{
	return stalloc(p - stacknext);
}

static inline void ststrdel(char *s)
{
	stfree(s);
}


#endif
