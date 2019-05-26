
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "mem.h"
#include "output.h"
#include "redir.h"

char basebuf[BUFSIZ+1];
struct parsefile basepf = {
	.prev = NULL,
	.lineno = 1,
	.fd = 0,
	.nleft = 0,
	.buf = basebuf,
	.nextc = basebuf,
	.unget = 0,
};
struct parsefile *parsefile = &basepf;
int whichprompt;

static void pushfile();
static int preadfd();
static void setinputfd(int fd, int push);
static int preadbuffer();

int pgetc()
{
	int c;

	if (parsefile->unget)
		return parsefile->lastc[--parsefile->unget];

	if (--parsefile->nleft >= 0)
		c = (signed char)*parsefile->nextc++;
	else
		c = preadbuffer();

	parsefile->lastc[1] = parsefile->lastc[0];
	parsefile->lastc[0] = c;

	return c;
}

void pungetc()
{
	if (parsefile->lastc[0] == '\n')
		parsefile->lineno--;
	if (++parsefile->unget > 2)
		die("pungetc: unget limit(%d) reached", 2);
}

static int preadfd()
{
	int nr;
	char *buf = parsefile->buf;
	parsefile->nextc = buf;

retry:
	nr = read(parsefile->fd, buf, BUFSIZ);

	if (nr < 0) {
		if (errno == EINTR)
			goto retry;
		if (parsefile->fd == 0 && errno == EWOULDBLOCK) {
			int flags = fcntl(0, F_GETFL, 0);
			if (flags >= 0 && flags & O_NONBLOCK) {
				flags &= ~O_NONBLOCK;
				if (fcntl(0, F_SETFL, flags) >= 0) {
					goto retry;
				}
			}
		}
	}
	return nr;
}

static int preadbuffer()
{
	char *q;
	int n, more;

	more = parsefile->nleft;
	if (more <= 0) {
again:
		if ((more = preadfd()) <= 0) {
			parsefile->nleft = 0;
			return PEOF;
		}
	}

	n = more;
	q = parsefile->nextc;

	for (;;) {
		int c;

		more--;
		c = *q;

		if (!c) {
			memmove(q, q+1, more);
			n--;
		} else {
			q++;

			if (c == '\n') {
				parsefile->nleft = q - parsefile->nextc - 1;
				break;
			}
		}

		if (more <= 0) {
			parsefile->nleft = q - parsefile->nextc - 1;
			if (parsefile->nleft < 0)
				goto again;
			break;
		}
	}
	parsefile->nleft = n-1;
	return (signed char)*parsefile->nextc++;
}

int setinputfile(const char *fname, int flags)
{
	int fd;

	if ((fd = open(fname, O_RDONLY)) < 0) {
		if (flags & INPUT_NOFILE_OK)
			goto out;
		/* TODO signal error */
	}
	if (fd < 10)
		fd = savefd(fd);
	setinputfd(fd, flags & INPUT_PUSH_FILE);
out:
	return fd;
}

static void setinputfd(int fd, int push)
{
	if (push) {
		pushfile();
		parsefile->buf = NULL;
	}
	parsefile->fd = fd;
	if (!parsefile->buf)
		parsefile->buf = xmalloc(BUFSIZ+1);
	parsefile->nleft = 0;
	plineno = 1;
}

void setinputstring(char *string)
{
	pushfile();
	parsefile->nextc = string;
	parsefile->nleft = strlen(string);
	parsefile->buf = NULL;
	plineno = 1;
}

static void pushfile()
{
	struct parsefile *pf;

	pf = xmalloc(sizeof(*pf));
	pf->prev = parsefile;
	pf->fd = -1;
	pf->unget = 0;
	parsefile = pf;
}

void popfile()
{
	struct parsefile *pf = parsefile;

	if (pf->fd >= 0)
		close(pf->fd);
	if (pf->buf)
		free(pf->buf);
	parsefile = pf->prev;
	free(pf);
}

void unwindfiles(struct parsefile *stop)
{
	while (parsefile != stop)
		popfile();
}

void popallfiles()
{
	unwindfiles(&basepf);
}

void closescript()
{
	popallfiles();
	if (parsefile->fd > 0) {
		close(parsefile->fd);
		parsefile->fd = 0;
	}
}

