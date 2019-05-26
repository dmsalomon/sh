
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "output.h"

void *xmalloc(size_t n)
{
	void *p;

	if (!(p = malloc(n)))
		die("xmalloc(%ld):", n);
	return p;
}

void *xrealloc(void *p, size_t n)
{
	if (!(p = realloc(p, n)))
		die("xrealloc():");
	return p;
}

char *xstrdup(const char *s)
{
	char *p = strdup(s);
	if (!p)
		die("xstrdup():");
	return p;
}

#define MINSIZE MEMALIGN(512)

struct stack_block {
	struct stack_block *prev;
	char space[MINSIZE];
};

struct stack_block stackbase;
struct stack_block *stackp = &stackbase;
char *stacknext = stackbase.space;
size_t stacknleft = MINSIZE;
char *sstrend = stackbase.space + MINSIZE;

void *stalloc(size_t n)
{
	char *p;
	size_t aligned;

	/* aligned = MEMALIGN(n); */
	aligned = n;
	if (aligned > stacknleft) {
		size_t len;
		size_t blocksize;
		struct stack_block *sp;

		blocksize = aligned;
		if (blocksize < MINSIZE)
			blocksize = MINSIZE;
		len = sizeof(struct stack_block) - MINSIZE + blocksize;
		if (len < blocksize)
			die("stalloc(): Out of space");
		sp = xmalloc(len);
		sp->prev = stackp;
		stacknext = sp->space;
		stacknleft = blocksize;
		sstrend = stacknext + blocksize;
		stackp = sp;
	}
	p = stacknext;
	stacknext += aligned;
	stacknleft -= aligned;
	return p;
}

void stfree(void *p)
{
	stacknleft += stacknext - (char *)p;
	stacknext = p;
}

void pushstackmark(struct stackmark *mark)
{
	mark->stackp = stackp;
	mark->stacknext = stacknext;
	mark->stacknleft = stacknleft;
}

void popstackmark(struct stackmark *mark)
{
	struct stack_block *sp;

	while (stackp != mark->stackp) {
		sp = stackp;
		stackp = sp->prev;
		free(sp);
	}
	stacknext = mark->stacknext;
	stacknleft = mark->stacknleft;
	sstrend = mark->stacknext + mark->stacknleft;
}

static void growstackblock(size_t min)
{
	size_t newlen;

	newlen = stacknleft * 2;
	if (newlen < stacknleft)
		die("growstackblock(): Out of space");
	min = MEMALIGN(min | 128);
	if (newlen < min)
		newlen += min;

	if (stacknext == stackp->space && stackp != &stackbase) {
		struct stack_block *sp;
		struct stack_block *prevstackp;
		size_t grosslen;

		sp = stackp;
		prevstackp = sp->prev;
		grosslen = newlen + sizeof(struct stack_block) - MINSIZE;
		sp = xrealloc(sp, grosslen);
		sp->prev = prevstackp;
		stackp = sp;
		stacknext = sp->space;
		stacknleft = newlen;
		sstrend = sp->space + newlen;
	} else {
		char *oldspace = stacknext;
		int oldlen = stacknleft;
		char *p = stalloc(newlen);

		/* free the space we just allocated */
		stacknext = memcpy(p, oldspace, oldlen);
		stacknleft += newlen;
	}
}

char *growstackstr(void)
{
	size_t len = stacknleft;

	growstackblock(0);
	return stacknext + len;
}

char *growstackto(size_t len)
{
	if (stacknleft < len)
		growstackblock(len);
	return stacknext;
}

char *sstrdup(const char *s)
{
	size_t len = strlen(s) + 1;
	return memcpy(stalloc(len), s, len);
}
