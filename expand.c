
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "eval.h"
#include "lexer.h"
#include "mem.h"
#include "parser.h"
#include "var.h"

#define IFS " \n\t"

static struct arg *cur;
static char *expdest;
static int quote;
static int len;

extern int rootpid;

static struct arg *expandarg(struct arg *);
static void procvalue(struct cmd *);
static void varvalue(const char *);
static void numappend(int);
static void expappend(const char *);
static void cappend(int);

struct arg *expandargs(struct arg *args)
{
	struct arg *head, *cur, *next;

	if (!args)
		return NULL;

	head = cur = expandarg(args);
	next = expandargs(args->next);

	if (!head)
		return next;

	while (cur->next)
		cur = cur->next;
	cur->next = next;

	return head;
}

static struct arg *expandarg(struct arg *arg)
{
	int c;
	char *s, *p;
	struct arg *first;
	struct cbinary *subst = arg->subst;

	len = 0;
	quote = 0;
	expdest = 0;
	first = cur = stalloc(sizeof(*cur));

	for (s = arg->text; (c = *s); ) {
		if (quote && quote == c) {
			quote = 0;
		} else if (!quote && (c == '\'' || c == '"')) {
			quote = c;
		} else if (c == '\\' && !quote) {
			cappend(*++s);
		} else if (c == '\\' && quote == '"') {
			if (!strchr("\\\"`", *(s+1))) {
				cappend(c);
			}
			cappend(*++s);
		} else if (c == '$' && quote != '\'') {
			c = *++s;
			if (c && strchr("$?", c))
				p = s+1;
			else if ((p = endofname(s)) == s) {
				cappend('$');
				continue;
			}
			c = *p;
			*p = '\0';
			varvalue(s);
			*p = c;
			s += (p-s-1);
		} else if (c == CTLSUBST) {
			assert(subst);
			procvalue(subst->left);
			subst = (struct cbinary *)subst->right;
		} else {
			cappend(c);
		}
		s++;
	}

	if (!len)
		cappend('\0');

	if (expdest != stacknext)
		STPUTC('\0', expdest);

	cur->text = ststrsave(expdest);
	cur->subst = NULL;
	cur->next = NULL;

	return first;
}

static void procvalue(struct cmd *cmd)
{
	char buf[512];
	int n, pip[2];
	pid_t pid;

	pipe(pip);

	if ((pid = dfork()) == 0) {
		close(pip[0]);
		dup2(pip[1], 1);
		_exit(eval(cmd));
	}
	close(pip[1]);

	while ((n = read(pip[0], buf, sizeof(buf)-1)) > 0) {
		buf[n] = '\0';
		expappend(buf);
	}
	close(pip[0]);
}

static void varvalue(const char *name)
{
	int num;
	char *p;

	switch (*name) {
	case '$':
		num = rootpid;
		goto num;
	case '?':
		num = exitstatus;
		goto num;
num:
		numappend(num);
		break;
	default:
		p = lookupvar(name);
		if (p)
			expappend(p);
		break;
	}
}

static void numappend(int n)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d", n);
	expappend(buf);
}

static void expappend(const char *s)
{
	if (quote == '"') {
		while (*s)
			cappend(*s++);
	}

	int inword = 0;

	const char *p;
	for (p = s; *p; p++) {
		if (strchr(IFS, *p)) {
			if (inword)
				cappend('\0');
			inword = 0;
			do {
				p++;
			} while (*p && strchr(IFS, *p));
			p--;
		} else {
			cappend(*p);
			inword = 1;
		}
	}
}

static void cappend(int c)
{
	if (!expdest) {
		STARTSTACKSTR(expdest);
	} else if (!STTOPC(expdest)) {
		cur->text = ststrsave(expdest);
		cur->next = stalloc(sizeof(*cur));
		cur = cur->next;
		STARTSTACKSTR(expdest);
	}
	STPUTC(c, expdest);
	len++;
}

