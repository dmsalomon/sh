
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "input.h"
#include "lexer.h"
#include "mem.h"

int yytoken = -1;
char *yytext;
unsigned int quoteflag;

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
	quoteflag = 0;

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
		if (quoteflag) printf(" quote");
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
			fprintf(stderr, "invalid input: %c %d\n", c, c);
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

	if (yytoken != TWORD || quoteflag)
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
	int c, peekc, str;
	char *ypp;

	str = peekc = quoteflag = 0;

	STARTSTACKSTR(yytext);
	ypp = yytext;

	while ((c = readchar()) != PEOF) {
		if (!str && strchr(" ()<>&\n\t\r\v;|", c)) {
			pungetc();
			if (c == '\n') plineno--;
			break;
		}

		/* string delimiter */
		if (c == '\'' || c == '"') {
			str = str == c ? '\0' : (str ? str : c);
			quoteflag = 1;
			if (!str || str == c)
				continue;
		}

		if (c == '\\' && !str) {
			c = pgetc();
			if (c == PEOF)
				break;
		}

		if (c == '\\' && str == '"') {
			peekc = pgetc();
			if (strchr("\\\"`", peekc)) {
				c = peekc;
				peekc = 0;
			}
		}

insert:
		STPUTC(c, ypp);

		if (peekc) {
			c = peekc;
			peekc = 0;
			goto insert;
		}
	}

	if (c == PEOF && str) {
		return TEOF;
	}

	STPUTC('\0', ypp);
	yytext = ststrsave(ypp);

	return TWORD;
}

void setprompt(int which)
{
	if (isatty(parsefile->fd)) {
		switch (which) {
		case 1:
			fprintf(stderr, "$ ");
			break;
		case 2:
			fprintf(stderr, "> ");
			break;
		default:
			fprintf(stderr, "+ ");
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
