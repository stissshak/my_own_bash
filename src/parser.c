// parser.c

#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

//////////////////////////////////////

// Auxiliary functions
int match(Token token, TokenType type){
	return token.type == type;
}

int is_redir_token(Token token){
	return (token.type >= TOKEN_LTLTLT && token.type <= TOKEN_GT);
}

RedirType get_redir_type(Token token){
	switch(token.type){
		case TOKEN_LTLTLT: return IN_BUF;
		case TOKEN_LTLT: return IN_END;
		case TOKEN_LEFT_AMP: return IN_ERR;
		case TOKEN_LTGT: return IN_NEW;
		case TOKEN_LT: return IN_OUT;
		case TOKEN_GTGT: return OUT_END;
		case TOKEN_RIGHT_AMP: return OUT_ERR;
		case TOKEN_GT: return OUT_NEW;
		default:
			fprintf(stderr, "Invalid redirection token\n");
			exit(1);
	}
}


int match_lparen(TokenType t){
	return (t == TOKEN_LLPAREN ||
		t == TOKEN_LPAREN  ||
		t == TOKEN_LBRACE);
}

int match_rparen(TokenType t){
	return (t == TOKEN_RRPAREN ||
		t == TOKEN_RPAREN  ||
		t == TOKEN_RBRACE);
}

TokenType get_matching_close(TokenType open){
	switch(open){
		case TOKEN_LLPAREN: return TOKEN_RRPAREN;
		case TOKEN_LPAREN: return TOKEN_RPAREN;
		case TOKEN_LBRACE: return TOKEN_RBRACE;
		default: return TOKEN_EOF;
	}
}

int is_operator(Token t){
	return (t.type == TOKEN_SEMICOLON ||
		t.type == TOKEN_AND ||
		t.type == TOKEN_OR ||
		t.type == TOKEN_BACK ||
		t.type == TOKEN_BAR ||
		t.type == TOKEN_PIPE_AMP ||
		match_lparen(t.type) ||
		match_rparen(t.type));
}


///////////////////////////////////////////////

// Wrapper
ASTNode *parse(Token *tokens){
	size_t index = 0;
	return parse_seq(tokens, &index);
}

// Parsing ; - with the lowest priotity
ASTNode *parse_seq(Token *tokens, size_t *index){
	ASTNode *left = parse_logic(tokens, index);

	while(!match(tokens[*index], TOKEN_EOF) && match(tokens[*index], TOKEN_SEMICOLON)){
		++*index;
		if(match(tokens[*index], TOKEN_EOF)){
			break;
		}
		ASTNode *right = parse_logic(tokens, index);
		left = create_binary(NODE_SEQ, left, right);
	}
	
	return left;
}

// Parsing || and &&
ASTNode *parse_logic(Token *tokens, size_t *index){
	ASTNode *left = parse_back(tokens, index);
	
	while(!match(tokens[*index], TOKEN_EOF) && (match(tokens[*index], TOKEN_AND) || match(tokens[*index], TOKEN_OR))){
		NodeType nt = match(tokens[*index], TOKEN_AND) ? NODE_AND : NODE_OR;
		++*index;
		ASTNode *right = parse_back(tokens, index);
		left = create_binary(nt, left, right);
	}

	return left;
}

ASTNode *parse_back(Token *tokens, size_t *index){
	ASTNode *left = parse_pipe(tokens, index);

	while(!match(tokens[*index], TOKEN_EOF) && match(tokens[*index], TOKEN_BACK)){
		++*index;
		left = create_unary(NODE_BACK, left);
	}
	
	return left;
}


// Parsing |
ASTNode *parse_pipe(Token *tokens, size_t *index){
	ASTNode *left = parse_group(tokens, index);
	
	while(!match(tokens[*index], TOKEN_EOF) && (match(tokens[*index], TOKEN_PIPE_AMP) || match(tokens[*index], TOKEN_BAR))){
		NodeType nt = NODE_PIPE;
		++*index;
		ASTNode *right = parse_group(tokens, index);
		left = create_binary(nt, left, right);
	}
	return left;
}


ASTNode *parse_group(Token *tokens, size_t *index){
	if(match_lparen(tokens[*index].type)){
		TokenType open = tokens[*index].type;
		TokenType close = get_matching_close(open);
		NodeType nd = (open == TOKEN_LLPAREN) ? NODE_ARIPHM : NODE_SUBSHELL;
		
		++*index;
		ASTNode *child = parse_seq(tokens, index);

		if(!match(tokens[*index], close)){
			fprintf(stderr, "Error: expected closing bracket\n");
			exit(1);
		}

		++*index;

		return create_unary(nd, child);
	}

	return parse_command(tokens, index);
}

// Parsing command with redirects
ASTNode *parse_command(Token *tokens, size_t *index){
	char **argv = NULL;
	int argc = 0;
	Redir *redirlist = NULL;

	while(!match(tokens[*index], TOKEN_EOF) && !is_operator(tokens[*index])){
		Token t = tokens[*index];

		if(is_redir_token(t)){
			RedirType rt = get_redir_type(t);
			++*index;

			if(match(tokens[*index], TOKEN_EOF) || !match(tokens[*index], TOKEN_WORD)){
				fprintf(stderr, "Error: expected file name after redir\n");
				exit(1);
			}
			
			add_redir(&redirlist, rt, tokens[*index].op);
			++*index;
		}
		
		else if(match(t, TOKEN_WORD)){
			argv = realloc(argv, sizeof(char*) * (argc+2));
			if(!argv){
				perror("parse_command");
				exit(1);
			}

			argv[argc] = strdup(t.op);
			argv[argc+1] = NULL;
			++argc;
			++*index;
		}
		else {
			fprintf(stderr, "Error: unexpected token in command\n");
			exit(1);
		}
	}

	if(argc == 0){
		return NULL;
	}

	return create_command(argc, argv, redirlist);
}


