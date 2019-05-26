
#ifndef CMD_H
#define CMD_H

#define CEXEC 0
#define CPIPE 1
#define CBANG 2
#define CAND 3
#define COR 4
#define CSUB 5
#define CREDIR 6
#define CWHILE 7
#define CUNTIL 8
#define CLIST 9
#define CBGND 10
#define CIF 11

extern const char *cmdname[];

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

/* constructors */
struct cmd *execcmd(void);
struct cmd *bincmd(int, struct cmd *, struct cmd *);
struct cmd *unrycmd(int, struct cmd *);
struct cmd *redircmd(struct cmd *, char *, int, int);
struct cmd *loopcmd(int, struct cmd *, struct cmd *);
struct cmd *ifcmd(struct cmd *, struct cmd *, struct cmd *);

#endif
