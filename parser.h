
#ifndef PARSER_H
#define PARSER_H

struct cmd *parseline(void);

#define CEOF ((struct cmd *)parseline)

#endif
