
#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "error.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "parser.h"
#include "var.h"

int show_tokens = 0;
int yytoken = TNL;
char *yytext;
struct cbinary *subst;

#define LEN(a) (sizeof(a) / sizeof(a[0]))

const char *tokname[] = {
    "TEOF",  "TNL",   "TSEMI", "TSEMIA", "TDSEMI", "TPIPE", "TAND",
    "TOR",   "TBGND", "TLPAR", "TRPAR",  "TLESS",  "TGRTR", "TDLSS",
    "TDGRT", "TWORD", "TWHLE", "TUNTL",  "TDO",    "TDONE", "TIF",
    "TTHEN", "TELSE", "TELIF", "TFI",    "TFOR",   "TIN",   "TCASE",
    "TESAC", "TLBRC", "TRBRC", "TBANG",  NULL,
};
static_assert(LEN(tokname) == TMAX + 1, "tokname should have length TMAX+1");

const char *toktxt[] = {
    "<EOF>", "<NL>", ";",  ";&",   ";;",   "|",    "&&",     "||",    "&",
    "(",     ")",    "<",  ">",    "<<",   ">>",   "<WORD>", "while", "until",
    "do",    "done", "if", "then", "else", "elif", "fi",     "for",   "in",
    "case",  "esac", "{",  "}",    "!",    NULL,
};
static_assert(LEN(toktxt) == TMAX + 1, "tokname should have length TMAX+1");

static int readchar(void);
static int readcharbnl(void);
static int word(void);

int nexttoken(void) {
  int c;

repeat:
  c = skipspaces();

  switch (c) {
  case PEOF:
    yytoken = TEOF;
    break;
  case ';':
    if ((c = readcharbnl()) == ';') {
      yytoken = TDSEMI;
    } else if (c == '&') {
      yytoken = TSEMIA;
    } else {
      pungetc();
      yytoken = TSEMI;
    }
    break;
  case '&':
    c = readcharbnl();
    if (c != '&') {
      pungetc();
      yytoken = TBGND;
    } else {
      yytoken = TAND;
    }
    break;
  case '|':
    c = readcharbnl();
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
    if ((c = readcharbnl()) == '>') {
      yytoken = TDGRT;
    } else {
      pungetc();
      yytoken = TGRTR;
    }
    break;
  case '<':
    if ((c = readcharbnl()) == '<') {
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
    // leave the newline behind to act as a separator
    consumeline(0);
    goto repeat;
  default:
    pungetc();
    yytoken = word();
    break;
  }

  if (yytoken != TWORD)
    yytext = sstrdup(toktxt[yytoken]);

  if (show_tokens) {
    printf("nexttoken(): %s `%s`", tokname[yytoken], yytext);
    if (yytoken == TWORD)
      printf(" size: %ld", strlen(yytext));
    printf("\n");
    fflush(stdout);
  }
  return yytoken;
}

/*
 * read a character and handle newline continuations
 */
static int readchar(void) {
  int c;

repeat:
  switch (c = pgetc()) {
  case '\n':
    newline();
    break;
  case PEOF:
    break;
  default:
    // skip garbage
    if (!isprint(c) && !ispunct(c) && !isspace(c))
      goto repeat;
    break;
  }

  return c;
}

/*
 * consumes all backslace-newline (bnl) sequences.
 */
static int readcharbnl() {
  int c;
  while ((c = readchar()) == '\\') {
    if (readchar() != '\n') {
      pungetc();
      break;
    }
    setprompt(2);
  }
  return c;
}

static inline int readchar_optbnl(int eatbnl) {
  return eatbnl ? readcharbnl() : readchar();
}

/*
 * do NOT skip newlines
 */
static inline int skipspaces(void) {
  int c;

  while ((c = readcharbnl()) != PEOF && strchr(" \v\r\t", c))
    ; /* advance */
  return c;
}

int checkwd(void) {
  if (yytoken != TWORD)
    return yytoken;

  for (int i = KWDOFFSET; i < TMAX; i++)
    if (strcmp(toktxt[i], yytext) == 0)
      return yytoken = i;

  return yytoken;
}

/*
 * grab a WORD
 */
static int word(void) {
  int c;
  char *ypp, *saveword;

  struct cbinary *cbase, **cpp;
  struct cunary *cu;

  int str = 0;
  int brace = 0;
  cpp = &cbase;

  STARTSTACKSTR(ypp);

  while ((c = readchar_optbnl(str != '\'')) != PEOF) {
    if (!str && !brace && strchr(" ()<>&\n\t\r\v;|", c)) {
      pungetc();
      break;
    }

    if (c == '$' && str != '\'') {
      if ((c = readcharbnl()) == '(') {
        /* save the stack string */
        int savelen = ypp - stacknext;
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
        ypp = growstackto(savelen + 1);
        if (savelen > 0) {
          memcpy(ypp, saveword, savelen);
          ypp += savelen;
        }
        c = CTLSUBST;
      } else {
        if (c == '{')
          brace = '}';
        c = '$';
        pungetc();
      }
    }

    /* string delimiter */
    if (c == '\'' || c == '"')
      str = str == c ? 0 : (str ? str : c);

    if (brace && c == brace)
      brace = 0;

    if (c == '\\' && str != '\'') {
      assert((c = readchar()) != '\n');
      STPUTC('\\', ypp);
      if (c == PEOF)
        break;
    }

    STPUTC(c, ypp);

    if (!brace && c == '\n')
      setprompt(2);
  }

  if (c == PEOF && str) {
    raiseerr("syntax: missing `%c`", str);
    /* not reached */
  }

  STPUTC('\0', ypp);
  yytext = ststrsave(ypp);
  *cpp = NULL;
  subst = cbase;

  return TWORD;
}

void setprompt(int which) {
  if (parsefile->isatty) {
    switch (which) {
    case 1:
      fprintf(stderr, "%s", ps1val);
      break;
    case 2:
      fprintf(stderr, "%s", ps2val);
      break;
    default:
      fprintf(stderr, "%s", ps4val);
      break;
    }
  }
}

/*
 * Clear out the rest of the line
 * useful for exceptions and comments
 * leaves newline as a separator
 */
void consumeline(int consumenl) {
  int c;

  while ((c = readchar()) != PEOF && c != '\n')
    ;

  if (!consumenl && c == '\n')
    pungetc();
}
