
#include <assert.h>
#include <stddef.h>

#include "cmd.h"
#include "mem.h"
#include "output.h"

const char *cmdname[] = {
	"exec",
	"pipe",
	"bang",
	"and",
	"or",
	"sub",
	"redir",
	"while",
	"until",
	"list",
	"background",
	"if",
	"func",
	NULL,
};

struct cmd* execcmd()
{
	struct cexec *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = CEXEC;
	cmd->argc = 0;
	cmd->argv = NULL;
	return (struct cmd*)cmd;
}

struct cmd *bincmd(int c, struct cmd *left, struct cmd *right)
{
	struct cbinary *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = c;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd*)cmd;
}

struct cmd *unrycmd(int c, struct cmd *subcmd)
{
	struct cunary *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = c;
	cmd->cmd = subcmd;
	return (struct cmd*)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, int mode, int fd)
{
	struct credir *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = CREDIR;
	cmd->cmd = subcmd;
	cmd->file = file;
	cmd->mode = mode;
	cmd->fd = fd;
	return (struct cmd*)cmd;
}

struct cmd *loopcmd(int op, struct cmd *cond, struct cmd *body)
{
	struct cloop *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = op;
	cmd->cond = cond;
	cmd->body = body;
	return (struct cmd*)cmd;
}

struct cmd *ifcmd(struct cmd *cond, struct cmd *ifp, struct cmd *elsep)
{
	struct cif *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = CIF;
	cmd->cond = cond;
	cmd->ifpart = ifp;
	cmd->elsepart = elsep;
	return (struct cmd*)cmd;
}

struct cmd *forcmd(char *var, struct arg *args, struct cmd *body)
{
	struct cfor *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = CFOR;
	cmd->var = var;
	cmd->list = args;
	cmd->body = body;
	return (struct cmd*)cmd;
}

struct cmd *funccmd(char *name, struct cmd *body)
{
	struct cfunc *cmd;
	cmd = stalloc(sizeof(*cmd));
	cmd->type = CFUNC;
	cmd->name = name;
	cmd->body = body;
	return (struct cmd*)cmd;
}

struct arg *copyargs(struct arg *ap)
{
	struct arg *bp, **bpp=&bp;

	while (ap) {
		*bpp = xmalloc(sizeof(**bpp));
		(*bpp)->text = xstrdup(ap->text);
		(*bpp)->subst = (struct cbinary*)copycmd((struct cmd*)ap->subst);
		bpp = &(*bpp)->next;
		ap = ap->next;
	}
	*bpp = NULL;
	return bp;
}

struct cmd *copycmd(struct cmd *c)
{
	if (!c)
		return NULL;

	struct cexec   *ce, *cce;
	struct cbinary *cb, *ccb;
	struct cunary  *cu, *ccu;
	struct credir  *cr, *ccr;
	struct cloop   *cl, *ccl;
	struct cif     *ci, *cci;
	struct cfor    *cf, *ccf;
	struct cfunc   *cfn, *ccfn;

	switch (c->type) {
	case CEXEC:
		ce = (struct cexec*)c;
		cce = xmalloc(sizeof(*cce));

		cce->type = ce->type;
		cce->argc = ce->argc;
		cce->args = copyargs(ce->args);
		return (struct cmd*)cce;

	case CPIPE:
	case CAND:
	case COR:
	case CLIST:
	case CBGND:
		cb = (struct cbinary*)c;
		ccb = xmalloc(sizeof(*ccb));

		ccb->type = cb->type;
		ccb->left = copycmd(cb->left);
		ccb->right = copycmd(cb->right);
		return (struct cmd*)ccb;

	case CBANG:
	case CSUB:
	case CBRC:
		cu = (struct cunary*)c;
		ccu = xmalloc(sizeof(*ccu));

		ccu->type = cu->type;
		ccu->cmd= copycmd(cu->cmd);
		return (struct cmd*)ccu;

	case CREDIR:
		cr = (struct credir*)c;
		ccr = xmalloc(sizeof(*ccr));

		ccr->type = cr->type;
		ccr->cmd = copycmd(cr->cmd);
		ccr->file = xstrdup(cr->file);
		ccr->mode = cr->mode;
		ccr->fd = cr->fd;
		return (struct cmd*)ccr;

	case CWHILE:
	case CUNTIL:
		cl = (struct cloop*)c;
		ccl = xmalloc(sizeof(*ccl));

		ccl->type = cl->type;
		ccl->cond = copycmd(cl->cond);
		ccl->body = copycmd(cl->body);
		return (struct cmd*)ccl;

	case CIF:
		ci = (struct cif*)c;
		cci = xmalloc(sizeof(*cci));

		cci->type = ci->type;
		cci->cond = copycmd(ci->cond);
		cci->ifpart = copycmd(ci->ifpart);
		cci->elsepart = copycmd(ci->elsepart);
		return (struct cmd*)cci;

	case CFOR:
		cf = (struct cfor*)c;
		ccf = xmalloc(sizeof(*ccf));

		ccf->type = cf->type;
		ccf->var = xstrdup(cf->var);
		ccf->list = copyargs(cf->list);
		ccf->body = copycmd(cf->body);
		return (struct cmd*)ccf;

	case CFUNC:
		cfn = (struct cfunc*)c;
		ccfn = xmalloc(sizeof(*ccfn));

		ccfn->type = cfn->type;
		ccfn->name = xstrdup(cfn->name);
		ccfn->body = copycmd(cfn->body);
		return (struct cmd*)ccfn;

	default:
		die("unknown command type: %d\n", c->type);
	}
}

void freeargs(struct arg *ap)
{
	struct arg *next;

	while (ap) {
		next = ap->next;
		free(ap->text);
		freecmd((struct cmd *)ap->subst);
		free(ap);
		ap = next;
	}
}

void freecmd(struct cmd *c)
{
	if (!c)
		return;

	struct cexec   *ce;
	struct cbinary *cb;
	struct cunary  *cu;
	struct credir  *cr;
	struct cloop   *cl;
	struct cif     *ci;
	struct cfor    *cf;
	struct cfunc   *cfn;

	switch (c->type) {
	case CEXEC:
		ce = (struct cexec*)c;

		freeargs(ce->args);
		break;

	case CPIPE:
	case CAND:
	case COR:
	case CLIST:
	case CBGND:
		cb = (struct cbinary*)c;

		freecmd(cb->left);
		freecmd(cb->right);
		break;

	case CBANG:
	case CSUB:
	case CBRC:
		cu = (struct cunary*)c;

		freecmd(cu->cmd);
		break;

	case CREDIR:
		cr = (struct credir*)c;

		freecmd(cr->cmd);
		free(cr->file);
		break;

	case CWHILE:
	case CUNTIL:
		cl = (struct cloop*)c;

		freecmd(cl->cond);
		freecmd(cl->body);
		break;

	case CIF:
		ci = (struct cif*)c;

		freecmd(ci->cond);
		freecmd(ci->ifpart);
		freecmd(ci->elsepart);
		break;

	case CFOR:
		cf = (struct cfor*)c;

		free(cf->var);
		freeargs(cf->list);
		freecmd(cf->body);
		break;

	case CFUNC:
		cfn = (struct cfunc*)c;

		free(cfn->name);
		freecmd(cfn->body);
		break;

	default:
		die("unknown command type: %d\n", c->type);
	}
}
