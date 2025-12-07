// exec.h

#pragma once

#include "ast.h"

////////////////////////////////

int execute(ASTNode*);
int execute_command(ASTNode *);
int execute_pipe(ASTNode*);
int execute_redirect(Redir *);
int execute_builtin(ASTNode*);

/////////////////////////////////

