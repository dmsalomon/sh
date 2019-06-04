
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "error.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "parser.h"

static struct cmd *parselist(void);
static struct cmd *parsecond(void);
static struct cmd *parsepipe(void);
static struct cmd *parsecmd(void);
static struct cmd *parsecmpcmd(void);
static struct cmd *parsecmplist(void);
static struct cmd *parseloop(void);
static struct cmd *parsedo(void);
static struct cmd *parseif(void);
static struct cmd *parseelse(void);
static struct cmd *parsesimple(void);
static struct cmd *parseredir(struct cmd *);

static void unexpected(void);

static int cmplistdone(void);
static int separator(void);
static int ionumber(const char *);

struct cmd *parseline(void)
{
	struct cmd *c;

	if (yytoken == TEOF) {
		return CEOF;
	}
	setprompt(1);
	if (nexttoken() == TNL)
		return NULL;
	if (yytoken == TEOF)
		return CEOF;

	c = parselist();

	if (yytoken != TEOF && yytoken != TNL)
		unexpected();

	return c;
}

static struct cmd *parselist(void)
{
	int op, cont;
	struct cmd *c;

	c = parsecond();

	while (yytoken == TSEMI || yytoken == TBGND) {
		op = yytoken == TSEMI ? CLIST : CBGND;
		nexttoken();
		cont = yytoken != TNL && yytoken != TEOF;
		c = bincmd(op, c, cont ? parsecond() : NULL);
	}
	return c;
}

static struct cmd *parsecond(void)
{
	int c;
	struct cmd *cmd;

	cmd = parsepipe();

	while (yytoken == TAND || yytoken == TOR) {
		c = yytoken == TAND ? CAND : COR;
		nexttoken();
		while(yytoken == TNL) {
			setprompt(2);
			nexttoken();
		}
		cmd = bincmd(c, cmd, parsepipe());
	}

	return cmd;
}

static struct cmd *parsepipe(void)
{
	int bang = 0;
	struct cmd *cmd;
	struct cmd *sub;

	if (yytoken == TWORD && strcmp(yytext, "!") == 0) {
		bang = 1;
		nexttoken();
	}

	cmd = parsecmd();
	if (!cmd) {
		unexpected();
	}

	while (yytoken == TPIPE) {
		nexttoken();
		while (yytoken == TNL) {
			setprompt(2);
			nexttoken();
		}
		sub = parsecmd();
		if (!sub)
			unexpected();
		cmd = bincmd(CPIPE, cmd, sub);
	}

	if (bang) {
		cmd = unrycmd(CBANG, cmd);
	}

	return cmd;
}

static struct cmd *parsecmd(void)
{
	switch (checkwd()) {
	case TLBRC:
	case TLPAR:
	case TWHLE:
	case TUNTL:
	case TIF:
		return parsecmpcmd();
	default:
		return parsesimple();
	}
}

static struct cmd *parsecmpcmd(void)
{
	struct cmd *cmd, *redir;

	switch (checkwd()) {
	case TLBRC:
	case TLPAR:
		cmd = parsesub();
		/* need to explictly consume TRPAR */
		nexttoken();
		break;
	case TWHLE:
		cmd = parseloop();
		break;
	case TUNTL:
		cmd = parseloop();
		break;
	case TIF:
		cmd = parseif();
		break;
	default:
		return NULL;
	}

	while ((redir = parseredir(cmd)) != cmd)
		cmd = redir;

	return cmd;
}

struct cmd *parsesub(void)
{
	int open;
	struct cmd *cmd;

	open = yytoken;
	nexttoken();

	cmd = unrycmd(open == TLPAR ? CSUB : CBRC, parsecmplist());

#if TLBRC+1 != TRBRC || TLPAR+1 != TRPAR
#error Token assumptions
#endif

	if (yytoken != open+1)
		unexpected();
	if (yytoken != TRPAR)
		nexttoken();

	return cmd;
}

static struct cmd *parsecmplist(void)
{
	struct cmd *c;

	while (yytoken == TNL) {
		setprompt(2);
		nexttoken();
	}

	c = parsecond();

	while (separator() && !cmplistdone()) {
		c = bincmd(CLIST, c, parsecond());
	}

	if (yytoken == TSEMI || yytoken == TBGND)
		nexttoken();
	while (yytoken == TNL)
		nexttoken();

	return c;
}

static struct cmd *parseloop(void)
{
	int looptype;
	struct cmd *cond, *body;

	if (yytoken != TWHLE && yytoken != TUNTL)
		unexpected();

	looptype = yytoken == TWHLE ? CWHILE : CUNTIL;
	nexttoken();

	cond = parsecmplist();
	if (yytoken != TDO)
		unexpected();

	body = parsedo();

	return loopcmd(looptype, cond, body);
}

static struct cmd *parsedo(void)
{
	struct cmd *c;

	if (yytoken != TDO)
		unexpected();
	nexttoken();

	c = parsecmplist();

	if (yytoken != TDONE)
		unexpected();
	nexttoken();

	return c;
}

static struct cmd *parseif(void)
{
	struct cmd *cond, *ifpart, *elsepart;

	if (yytoken != TIF)
		unexpected();
	nexttoken();

	cond = parsecmplist();
	if (yytoken != TTHEN)
		unexpected();
	nexttoken();

	ifpart = parsecmplist();

	if (yytoken == TELIF || yytoken == TELSE)
		elsepart = parseelse();
	else if (yytoken == TFI)
		elsepart = NULL;
	else
		unexpected();

	nexttoken();

	return ifcmd(cond, ifpart, elsepart);
}

static struct cmd *parseelse(void)
{
	struct cmd *cond, *ifpart, *elsepart;

	if (yytoken != TELIF && yytoken != TELSE)
		unexpected();

	if (yytoken == TELSE) {
		nexttoken();
		return parsecmplist();
	}

	nexttoken();
	cond = parsecmplist();
	if (yytoken != TTHEN)
		unexpected();
	nexttoken();

	ifpart = parsecmplist();

	if (yytoken == TELIF || yytoken == TELSE) {
		nexttoken();
		elsepart = parsecmplist();
	} else {
		elsepart = NULL;
	}

	return ifcmd(cond, ifpart, elsepart);
}

static struct cmd *parsesimple(void)
{
	struct cmd *cmd;
	struct cexec *ecmd;
	struct arg **app;

	cmd = execcmd();
	ecmd = (struct cexec*)cmd;
	app = &ecmd->args;

	cmd = parseredir(cmd);

	while (yytoken == TWORD) {
		*app = stalloc(sizeof(**app));
		(*app)->text = yytext;
		(*app)->subst = subst;
		app = &(*app)->next;
		ecmd->argc++;
		nexttoken();
		cmd = parseredir(cmd);
	}
	*app = NULL;
	if (!ecmd->argc)
		return NULL;
	return cmd;
}

static struct cmd *parseredir(struct cmd *cmd)
{
	int fd;
	int op;

	for (;;) {
		switch (yytoken) {
		case TLESS:
			fd = 0;
			break;
		case TGRTR:
			fd = 1;
			break;
		case TDGRT:
			fd = 1;
			break;
		case TWORD:
			op = skipspaces();
			pungetc();
			if (op != '<' && op != '>')
				goto out;
			if ((fd = ionumber(yytext)) < 0)
				goto out;
			nexttoken();
			break;
		default:
			goto out;
		}

		switch (yytoken) {
		case TLESS:
			op = '<';
			break;
		case TGRTR:
			op = '>';
			break;
		case TDGRT:
			op = '+';
			break;
		default:
			unexpected();
		}

		nexttoken();

		if (yytoken != TWORD) {
			unexpected();
			goto out;
		}

		cmd = redircmd(cmd, yytext, op, fd);
		nexttoken();
	}
out:
	return cmd;
}

static int separator(void)
{
	int sep = 0;

	if (yytoken == TSEMI || yytoken == TBGND) {
		nexttoken();
		sep = 1;
	}

	while (yytoken == TNL) {
		setprompt(2);
		nexttoken();
		sep = 1;
	}

	return sep;
}

static int cmplistdone(void)
{
	switch (checkwd()) {
	case TRPAR:
	case TRBRC:
	case TDO:
	case TDONE:
	case TTHEN:
	case TELSE:
	case TELIF:
	case TFI:
		return 1;
	default:
		return 0;
	}
}

int isassignment(const char *word)
{
	char *p;

	p = endofname(word);
	return p != word && *p == '=';
}

char *endofname(const char *name)
{
	char *p;

	p = (char *)name;
	if (!is_name(*p))
		return p;
	while (*++p)
		if (!is_in_name(*p))
			break;
	return p;
}

static int ionumber(const char *word)
{
	return (isdigit(word[0]) && !word[1]) ? (word[0] - '0') : (-1);
}

static void unexpected()
{
	int c;

	if (yytoken == TEOF)
		yytoken = TNL;

	if (yytoken != TNL)
		while ((c = pgetc()) != PEOF && c != '\n')
			;

	raiseerr("%d: syntax: `%s` unexpected", plineno, yytext);
}
