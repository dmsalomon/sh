
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "input.h"
#include "mem.h"
#include "parser.h"
#include "var.h"

#define VTABSIZE 39

char linenovar[sizeof("LINENO=")+sizeof(int)*8+2] = "LINENO=";

struct var varinit[] = {
	{ 0, VSTSTAT|VTXSTAT, "PS1=$ ",  0 },
	{ 0, VSTSTAT|VTXSTAT, "PS2=> ",  0 },
	{ 0, VSTSTAT|VTXSTAT, "PS4=+ ",  0 },
	{ 0, VSTSTAT|VTXSTAT, linenovar, 0 },
};

static struct var *vartab[VTABSIZE];

static struct var **hashvar(const char *);
static struct var **findvar(struct var **, const char *);

#ifndef HAVE_STRCHRNUL
static char *strchrnul(const char *s, int c)
{
	while (*s && *s != c)
		s++;
	return (char *)s;
}
#endif

void initvar(void)
{
	extern char **environ;

	char **envp, *p;
	struct var *vp;
	struct var *end;
	struct var **vpp;

	vp = varinit;
	end = vp + sizeof(varinit) / sizeof(varinit[0]);

	do {
		vpp = hashvar(vp->text);
		vp->next = *vpp;
		*vpp = vp;
	} while (++vp < end);

	if (!geteuid())
		vps1.text = "PS1=# ";

	for (envp = environ; *envp; envp++) {
		p = endofname(*envp);
		if (p != *envp && *p == '=') {
			setvareq(*envp, VEXPORT|VTXSTAT);
		}
	}

	static char ppid[32] = "PPID=";
	snprintf(ppid+5, sizeof(ppid)-5, "%ld", (long)getppid());
	setvareq(ppid, VTXSTAT);
}

char *lookupvar(const char *name)
{
	struct var *v;

	if ((v = *findvar(hashvar(name), name)) && !(v->flags & VUNSET)) {
		if (v == &vlineno && v->text == linenovar) {
			snprintf(linenovar+7, sizeof(linenovar)-7, "%d", plineno-1);
		}
		return strchrnul(v->text, '=') + 1;
	}
	return NULL;
}

struct var *setvar(const char *name, const char *val, int flags)
{
	char *p, *q;
	size_t namelen, vallen;
	char *nameeq;
	struct var *vp;

	q = endofname(name);
	p = strchrnul(q, '=');
	namelen = p - name;
	if (namelen == 0 || p != q)
		raiseerr("%.*s: bad variable name", namelen, name);

	vallen = 0;
	if (val)
		vallen = strlen(val);
	else
		flags |= VUNSET;

	INTOFF;
	p = memcpy(nameeq = xmalloc(namelen+vallen+2), name, namelen);
	p += namelen;
	if (val) {
		*p++ = '=';
		memcpy(p, val, vallen);
		p += vallen;
	}
	*p = '\0';
	vp = setvareq(nameeq, flags | VNOSAVE);
	INTON;

	return vp;
}

struct var *setvareq(char *s, int flags)
{
	struct var *vp, **vpp;

	vpp = findvar(hashvar(s), s);
	vp = *vpp;

	if (vp) {
		if (vp->flags & VRDONLY) {
			const char *n;

			if (flags & VNOSAVE)
				free(s);
			n = vp->text;
			raiseerr("%.*s: is read only", strchrnul(n, '=')-n, n);
		}

		if (flags & VNOSET)
			goto out;

		if (vp->func && (flags & VNOFUNC) == 0)
			vp->func(strchrnul(s, '=') + 1);

		if ((vp->flags & (VTXSTAT|VSTACK)) == 0)
			free(vp->text);

		if (((flags & (VEXPORT|VRDONLY|VSTSTAT|VUNSET)) |
			(vp->flags & VSTSTAT)) == VUNSET) {
			*vpp = vp->next;
			free(vp);
out_free:
			if ((flags & (VTXSTAT|VSTACK|VNOSAVE)) == VNOSAVE)
				free(s);
			goto out;
		}

		flags |= vp->flags & ~(VTXSTAT|VSTACK|VNOSAVE|VUNSET);
	} else {
		if (flags & VNOSET)
			goto out;
		if ((flags & (VEXPORT|VRDONLY|VSTSTAT|VUNSET)) == VUNSET)
			goto out_free;
		vp = xmalloc(sizeof(*vp));
		vp->next = *vpp;
		vp->func = NULL;
		*vpp = vp;
	}
	if (!(flags & (VTXSTAT|VSTACK|VNOSAVE)))
		s = xstrdup(s);
	vp->text = s;
	vp->flags = flags;
	if (flags & VEXPORT)
		putenv(s);
out:
	return vp;
}

int export_builtin(struct cexec *cmd)
{
	/* lets not bother listing the variables
	 * forget POSIX, at least for now*/

	int flag = cmd->argv[0][0] == 'r' ? VRDONLY : VEXPORT;
	char *ap, **app, *p;
	struct var *vp;

	for (app = &cmd->argv[1]; (ap = *app); app++) {
		if ((p = strchr(ap, '=')) != NULL) {
			p++;
		} else {
			if ((vp = *findvar(hashvar(ap), ap))) {
				vp->flags |= flag;
				continue;
			}
		}
		setvar(ap, p, flag);
	}

	return 0;
}

static struct var **hashvar(const char *p)
{
	unsigned int hashval;

	hashval = ((unsigned char) *p) << 4;
	while (*p && *p != '=')
		hashval += (unsigned char) *p++;
	return &vartab[hashval & VTABSIZE];
}

int varcmp(const char *p, const char *q)
{
	int c, d;

	while ((c = *p) == (d = *q)) {
		if (!c ||c == '=')
			goto out;
		p++, q++;
	}
	if (c == '=')
		c = 0;
	if (d == '=')
		d = 0;
out:
	return c - d;
}

static struct var **findvar(struct var **vpp, const char *name)
{
	for (; *vpp; vpp = &(*vpp)->next)
		if (varequal((*vpp)->text, name))
			break;

	return vpp;
}
