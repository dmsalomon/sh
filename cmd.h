
#ifndef CMD_H
#define CMD_H

#define CEXEC 0
#define CPIPE 1
#define CBANG 2
#define CAND 3
#define COR 4
#define CSUB 5
#define CBRC 6
#define CREDIR 7
#define CWHILE 8
#define CUNTIL 9
#define CLIST 10
#define CBGND 11
#define CIF 12
#define CFOR 13
#define CFUNC 14

extern const char *cmdname[];

#define CEOF ((struct cmd*)cmdname)

struct cmd {
	int type;
};

struct cexec {
	int type;
	int argc;
	char **argv;
	struct arg *args;
};

struct arg {
	char *text;
	struct arg *next;
	struct cbinary *subst;
};

struct cbinary {
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct cunary {
	int type;
	struct cmd *cmd;
};

struct credir {
	int type;
	struct cmd *cmd;
	char *file;
	int mode;
	int fd;
};

struct cloop {
	int type;
	struct cmd *cond;
	struct cmd *body;
};

struct cif {
	int type;
	struct cmd *cond;
	struct cmd *ifpart;
	struct cmd *elsepart;
};

struct cfor {
	int type;
	char *var;
	struct arg *list;
	struct cmd *body;
};

struct cfunc {
	int type;
	char *name;
	struct cmd *body;
};

/* constructors */
struct cmd *execcmd(void);
struct cmd *bincmd(int, struct cmd *, struct cmd *);
struct cmd *unrycmd(int, struct cmd *);
struct cmd *redircmd(struct cmd *, char *, int, int);
struct cmd *loopcmd(int, struct cmd *, struct cmd *);
struct cmd *ifcmd(struct cmd *, struct cmd *, struct cmd *);
struct cmd *forcmd(char *, struct arg *, struct cmd *);
struct cmd *funccmd(char *, struct cmd *);

/* deepcopy */
struct arg *copyargs(struct arg *);
struct cmd *copycmd(struct cmd *);
void freeargs(struct arg *);
void freecmd(struct cmd *);

#endif
