
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "mem.h"
#include "output.h"
#include "redir.h"

struct redirtab {
	struct redirtab *next;
	int fd;
	int save;
};

struct redirtab *redirlist;

int pushredirect(struct credir *cr)
{
	struct redirtab *rtab;

	int mode, ofd;

	switch (cr->mode) {
	case '<':
		mode = O_RDONLY;
		break;
	case '>':
		mode = O_CREAT | O_TRUNC | O_WRONLY;
		break;
	case '+':
		mode = O_CREAT | O_APPEND | O_WRONLY;
		break;
	default:
		die("unknown redirection");
		break;
	}

	if ((ofd = open(cr->file, mode, 0666)) < 0) {
		perrorf("%s:", cr->file);
		goto bad;
	}

	rtab = stalloc(sizeof(*rtab));
	rtab->fd = cr->fd;
	rtab->save = fcntl(cr->fd, F_GETFD) != (-1) ? savefd(cr->fd) : (-1);

	if (ofd != cr->fd) {
		if (dup2(ofd, cr->fd) < 0) {
			perrorf("pushredirect: %d:", cr->fd);
			goto del;
		}
		close(ofd);
	}

	rtab->next = redirlist;
	redirlist = rtab;

	return cr->fd;

del:
	if (rtab->save)
		close(rtab->save);
	stfree(rtab);
bad:
	return -1;
}

void popredirect(void)
{
	struct redirtab *rtab;

	if (!redirlist)
		return;

	rtab = redirlist;
	redirlist = redirlist->next;

	if (rtab->save >= 0) {
		if (dup2(rtab->save, rtab->fd) < 0)
			perrorf("popredir: %d %d:", rtab->save, rtab->fd);
		close(rtab->save);
	} else {
		close(rtab->fd);
	}
}

void unwindredir(void)
{
	while (redirlist)
		popredirect();
}

int savefd(int fd)
{
	int newfd;
	int err;

	newfd = fcntl(fd, F_DUPFD, 10);
	err = newfd < 0 ? errno : 0;
	if (err != EBADF) {
		close(fd);
		if (err)
			die("%d:", fd);
		else
			fcntl(newfd, F_SETFD, FD_CLOEXEC);
	}
	return newfd;
}

