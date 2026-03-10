
#ifndef LEXER_H
#define LEXER_H

#include <assert.h>
#define TEOF   0
#define TNL    1
#define TSEMI  2
#define TSEMIA 3
#define TDSEMI 4
#define TPIPE  5
#define TAND   6
#define TOR    7
#define TBGND  8
#define TLPAR  9
#define TRPAR  10
#define TLESS  11
#define TGRTR  12
#define TDLSS  13
#define TDGRT  14
#define TWORD  15
#define TWHLE  16
#define TUNTL  17
#define TDO    18
#define TDONE  19
#define TIF    20
#define TTHEN  21
#define TELSE  22
#define TELIF  23
#define TFI    24
#define TFOR   25
#define TIN    26
#define TCASE  27
#define TESAC  28
#define TLBRC  29
#define TRBRC  30
#define TBANG  31
#define TMAX   32

#define KWDOFFSET 16
static_assert(KWDOFFSET == TWHLE, "Keyword sanity check");

#include "cmd.h"

extern int show_tokens;
extern int yytoken;
extern char *yytext;
extern struct cbinary *subst;
extern const char *tokname[];
extern const char *toktxt[];

int nexttoken(void);
int skipspaces(void);
int checkwd(void);
void setprompt(int);
void consumeline(int);

#define CTLSUBST (-125)

#endif
