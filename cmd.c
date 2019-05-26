
#include <assert.h>
#include <stddef.h>

#include "cmd.h"
#include "mem.h"

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

