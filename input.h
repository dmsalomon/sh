
#ifndef INPUT_H
#define INPUT_H

enum {
	INPUT_PUSH_FILE = 1,
	INPUT_NOFILE_OK = 2,
};

struct parsefile {
	struct parsefile *prev;
	int lineno;
	int fd;
	int nleft;
	char *nextc;
	char *buf;
	int lastc[2];
	int unget;
};

extern struct parsefile *parsefile;

#define plineno (parsefile->lineno)

int pgetc();
void pungetc();
int setinputfile(const char *, int);
void popfile();
void unwindfiles(struct parsefile *);
void popallfiles();
void closescript();

#define PEOF (-1)

static inline void newline(void)
{
	if (++plineno == 0)
		fprintf(stderr, "wtf newline\n");
}

#endif
