// exec.h

#pragma once

#include "ast.h"

////////////////////////////////

int execute(ASTNode*, int);
int execute_command(ASTNode *, int);
int execute_pipe(ASTNode*, int);
int execute_redirect(Redir *);
int execute_builtin(ASTNode*);

/////////////////////////////////

