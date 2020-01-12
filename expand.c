
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "error.h"
#include "eval.h"
#include "expand.h"
#include "lexer.h"
#include "mem.h"
#include "parser.h"
#include "sh.h"
#include "var.h"

static struct arg *cur;
static char *expdest;
static int quote;
static int len;
static int filename;

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
	int wasquoted = 0;
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
			wasquoted = 1;
		} else if (c == '\\' && !quote) {
			if ((c = *++s) != '\n')
				cappend(c);
		} else if (c == '\\' && quote == '"') {
			if (!strchr("$\\\"`\n", (c = *++s)))
				cappend('\\');
			if (c != '\n')
				cappend(c);
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

	if (len == 0) {
		if (wasquoted) {
			cappend('\0');
		} else {
			stfree(first);
			return NULL;
		}
	} else if (STTOPC(expdest) != '\0') {
		cappend('\0');
	}

	cur->text = ststrsave(expdest);
	cur->subst = NULL;
	cur->next = NULL;

	return first;
}

static void procvalue(struct cmd *cmd)
{
	char buf[BUFSIZ];
	int n, lastc, pip[2];
	pid_t pid;

	pipe(pip);

	INTOFF;

	if ((pid = dfork()) == 0) {
		close(pip[0]);
		dup2(pip[1], 1);
		_exit(eval(cmd));
	}
	close(pip[1]);

	lastc = 0;
	while ((n = read(pip[0], buf, sizeof(buf)-1)) > 0) {
		buf[n] = '\0';
		if (lastc && !strchr(IFS, lastc) && strchr(IFS, buf[0]))
			cappend('\0');
		expappend(buf);
		lastc = buf[n-1];
	}
	close(pip[0]);
	waitsh(pid);
	INTON;

	if ((quote||filename) && expdest && lastc == '\n' && STTOPC(expdest) == '\n') {
		expdest--;
		len--;
	}
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
	if (quote == '"' || filename) {
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

char *expfilename(struct arg *arg) {
	filename = 1;
	arg = expandarg(arg);
	filename = 0;
	return arg->text;
}

static void cappend(int c)
{
	if (!expdest) {
		STARTSTACKSTR(expdest);
	} else if (STTOPC(expdest) == '\0') {
		cur->text = ststrsave(expdest);
		cur->next = stalloc(sizeof(*cur));
		cur = cur->next;
		STARTSTACKSTR(expdest);
	}
	STPUTC(c, expdest);
	len++;
}

