
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "output.h"
#include "parser.h"

static struct cmd *parselist(void);
static struct cmd *parsecond(void);
static struct cmd *parsepipe(void);
static struct cmd *parsecmd(void);
static struct cmd *parsecmpcmd(void);
static struct cmd *parsesub(void);
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

jmp_buf beginparse;
extern int exitstatus;

struct cmd *parseline(void)
{
	struct cmd *c;

	if (setjmp(beginparse)) {
		return NULL;
	}

	if (yytoken == TEOF)
		return CEOF;
	do {
		setprompt(1);
	} while (nexttoken() == TNL);
	if (yytoken == TEOF)
		return CEOF;

	c = parselist();

	if (yytoken != TEOF && yytoken != TNL)
		unexpected();

	return c;
}

static struct cmd *parselist(void)
{
	int op;
	struct cmd *c;

	c = parsecond();

	while (yytoken == TSEMI || yytoken == TBGND) {
		op = yytoken == TSEMI ? CLIST : CBGND;
		c = bincmd(op, c, nexttoken() == TNL ? NULL : parsecond());
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
	if (checkwd() != TWORD)
		return parsecmpcmd();

	return parsesimple();
}

static struct cmd *parsecmpcmd(void)
{
	struct cmd *cmd, *redir;

	switch (yytoken) {
	case TLPAR:
		cmd = parsesub();
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

static struct cmd *parsesub(void)
{
	struct cmd *cmd;

	if (yytoken != TLPAR)
		unexpected();

	nexttoken();

	cmd = unrycmd(CSUB, parsecmplist());

	if (yytoken != TRPAR)
		unexpected();
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
	if (yytoken == TRPAR)
		return 1;
	if (yytoken == TWORD) {
		if (strcmp(yytext, "do") == 0)
			yytoken = TDO;
		else if (strcmp(yytext, "done") == 0)
			yytoken = TDONE;
		else if (strcmp(yytext, "then") == 0)
			yytoken = TTHEN;
		else if (strcmp(yytext, "else") == 0)
			yytoken = TELSE;
		else if (strcmp(yytext, "elif") == 0)
			yytoken = TELIF;
		else if (strcmp(yytext, "fi") == 0)
			yytoken = TFI;
		if (yytoken != TWORD)
			return 1;
	}
	return 0;
}

static int ionumber(const char *word)
{
	int n = word[0] - '0';
	return (isdigit(word[0]) && !word[1]) ? n : (-1);
}

static void unexpected()
{
	int c;

	perrorf("%d: syntax: `%s` unexpected", plineno, yytext);

	if (yytoken == TEOF)
		yytoken = TNL;

	if (yytoken != TNL)
		while ((c = pgetc()) != PEOF && c != '\n')
			;

	exitstatus = 2;

	longjmp(beginparse, 1);
}
