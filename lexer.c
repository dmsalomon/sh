
#include <alloca.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "parser.h"
#include "var.h"

int yytoken = TNL;
char *yytext;
struct cbinary *subst;

const char *tokname[] = {
	"TEOF",
	"TNL",
	"TSEMI",
	"TPIPE",
	"TAND",
	"TOR",
	"TBGND",
	"TLPAR",
	"TRPAR",
	"TLESS",
	"TGRTR",
	"TDLSS",
	"TDGRT",
	"TWORD",
	"TNOT",
	"TWHLE",
	"TUNTL",
	"TDO",
	"TDONE",
	"TIF",
	"TTHEN",
	"TELSE",
	"TELIF",
	"TFI",
	"TFOR",
	"TIN",
	"TLBRC",
	"TRBRC",
	NULL,
};

const char *toktxt[] = {
	"<EOF>",
	"<NL>",
	";",
	"|",
	"&&",
	"||",
	"&",
	"(",
	")",
	"<",
	">",
	"<<",
	">>",
	"word",
	"!",
	"while",
	"until",
	"do",
	"done",
	"if",
	"then",
	"else",
	"elif",
	"fi",
	"for",
	"in",
	"{",
	"}",
	NULL,
};

static int readchar(void);
static int word(void);
static void comment(void);

int nexttoken(void)
{
	int c;

	c = skipspaces();

	switch (c) {
	case PEOF:
		yytoken = TEOF;
		break;
	case ';':
		yytoken = TSEMI;
		break;
	case '&':
		c = readchar();
		if (c != '&') {
			pungetc();
			yytoken = TBGND;
		} else {
			yytoken = TAND;
		}
		break;
	case '|':
		c = readchar();
		if (c != '|') {
			pungetc();
			yytoken = TPIPE;
		} else {
			yytoken = TOR;
		}
		break;
	case '(':
		yytoken = TLPAR;
		break;
	case ')':
		yytoken = TRPAR;
		break;
	case '>':
		if ((c = readchar()) == '>') {
			yytoken = TDGRT;
		} else {
			pungetc();
			yytoken = TGRTR;
		}
		break;
	case '<':
		if ((c = readchar()) == '<') {
			yytoken = TDLSS;
		} else {
			pungetc();
			yytoken = TLESS;
		}
		break;
	case '\n':
		yytoken = TNL;
		break;
	case '#':
		comment();
		return nexttoken();
	default:
		pungetc();
		yytoken = word();
		break;
	}

	if (yytoken != TWORD)
		yytext = sstrdup(toktxt[yytoken]);

	if (0) {
		printf("nexttoken(): %s `%s`", tokname[yytoken], yytext);
		if (yytoken == TWORD) printf(" size: %ld", strlen(yytext));
		printf("\n");
		fflush(stdout);
	}
	return yytoken;
}

/*
 * read a character and handle newline continuations
 */
static int readchar(void)
{
	int c;

repeat:
	switch (c = pgetc()) {
	case '\\':
		if ((c = pgetc()) == '\n') {
			newline();
			setprompt(2);
			goto repeat;
		}
		pungetc();
		c = '\\';
		break;
	case '\n':
		newline();
		break;
	case PEOF:
		break;
	default:
		if (!isprint(c) && !ispunct(c) && !isspace(c))
			goto repeat;
		break;
	}

	return c;
}

/*
 * do NOT skip newlines
 */
int skipspaces(void)
{
	int c;

	while ((c = readchar()) != PEOF && strchr(" \v\r\t", c))
		; /* advance */
	return c;
}

int checkwd(void)
{
	const char **p;

	if (yytoken != TWORD)
		return yytoken;

	for (p = &toktxt[KWDOFFSET]; *p; p++)
		if (strcmp(*p, yytext) == 0)
			return yytoken = (p - toktxt);

	return yytoken;
}

/*
 * grab a WORD
 */
static int word(void)
{
	int c, str, savelen;
	char *ypp, *saveword;

	struct cbinary *cbase, **cpp;
	struct cunary *cu;

	str = 0;
	cpp = &cbase;

	STARTSTACKSTR(ypp);

	while ((c = readchar()) != PEOF) {
		if (!str && strchr(" ()<>&\n\t\r\v;|", c)) {
			pungetc();
			break;
		}

		if (c == '$' && str != '\'') {
			if ((c = readchar()) == PEOF) {
				STPUTC('$', ypp);
				break;
			}
			if (c == '(') {
				/* save the stack string */
				savelen = ypp - stacknext;
				if (savelen > 0) {
					saveword = alloca(savelen);
					memcpy(saveword, stacknext, savelen);
				}

				/* parse cmd substitution */
				yytoken = TLPAR;
				cu = (struct cunary *)parsesub();
				*cpp = (struct cbinary *)bincmd(CLIST, cu->cmd, NULL);
				cpp = (struct cbinary **)&(*cpp)->right;

				/* restore the stack string */
				ypp = growstackto(savelen+1);
				if (savelen > 0) {
					memcpy(ypp, saveword, savelen);
					ypp += savelen;
				}
				c = CTLSUBST;
			} else {
				STPUTC('$', ypp);
			}
		}

		/* string delimiter */
		if (c == '\'' || c == '"')
			str = str == c ? '\0' : (str ? str : c);

		if (c == '\\' && str != '\'') {
			STPUTC('\\', ypp);
			if ((c = pgetc()) == PEOF)
				break;
		}

		STPUTC(c, ypp);

		if (c == '\n')
			setprompt(2);
	}

	if (c == PEOF && str) {
		return TEOF;
	}

	STPUTC('\0', ypp);
	yytext = ststrsave(ypp);
	*cpp = NULL;
	subst = cbase;

	return TWORD;
}

void setprompt(int which)
{
	if (isatty(parsefile->fd)) {
		switch (which) {
		case 1:
			fprintf(stderr, ps1val);
			break;
		case 2:
			fprintf(stderr, ps2val);
			break;
		default:
			fprintf(stderr, ps4val);
			break;
		}
	}
}


/*
 * consumes a comment
 * leave newline behind as separator
 */
static void comment(void)
{
	int c;

	while ((c = pgetc()) != PEOF && c != '\n')
		;

	if (c == '\n')
		pungetc();
}
