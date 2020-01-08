
#ifndef LEXER_H
#define LEXER_H

#define TEOF  0
#define TNL   1
#define TSEMI 2
#define TPIPE 3
#define TAND  4
#define TOR   5
#define TBGND 6
#define TLPAR 7
#define TRPAR 8
#define TLESS 9
#define TGRTR 10
#define TDLSS 11
#define TDGRT 12
#define TWORD 13
#define TNOT  14
#define TWHLE 15
#define TUNTL 16
#define TDO   17
#define TDONE 18
#define TIF   19
#define TTHEN 20
#define TELSE 21
#define TELIF 22
#define TFI   23
#define TFOR  24
#define TIN   25
#define TLBRC 26
#define TRBRC 27
#define TMAX  28

#define KWDOFFSET 14

#include "cmd.h"

extern int yytoken;
extern char *yytext;
extern struct cbinary *subst;
extern const char *tokname[];
extern const char *toktxt[];

int nexttoken(void);
int skipspaces(void);
int checkwd(void);
void setprompt(int);

#define CTLSUBST (-125)

#endif
