#pragma once

#include "token.h"
#include "ast.h"

#include <stddef.h>

///////////////////////////////////////

ASTNode *parse(Token *);
ASTNode *parse_seq(Token *, size_t *);
ASTNode *parse_logic(Token *, size_t *);
ASTNode *parse_pipe(Token *, size_t *);
ASTNode *parse_group(Token *, size_t *);
ASTNode *parse_command(Token *, size_t *);

int match(Token, TokenType);
int is_redir_token(Token);
RedirType get_redir_type(Token);
int match_lparen(TokenType);
int match_rparen(TokenType);
TokenType get_matching_close(TokenType);
int is_operator(Token);

//////////////////////////////////////

