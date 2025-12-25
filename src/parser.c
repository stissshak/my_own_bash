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
	return (token.type >= TOKEN_LTLTLT && token.type <= TOKEN_AMP_GT);
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
		case TOKEN_AMP_GTGT: return OUT_ALL_END;
                case TOKEN_AMP_GT: return OUT_ALL;
		default:
			fprintf(stderr, "Invalid redirection token\n");
			exit(1);
	}
}


int match_lparen(TokenType t){
	return (t == TOKEN_LPAREN  ||
		t == TOKEN_LBRACE);
}

int match_rparen(TokenType t){
	return (t == TOKEN_RPAREN  ||
		t == TOKEN_RBRACE);
}

TokenType get_matching_close(TokenType open){
	switch(open){
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
		match_lparen(t.type) ||
		match_rparen(t.type));
}


///////////////////////////////////////////////

// Wrapper
ASTNode *parse(Token *tokens){
	size_t index = 0;
	
    if(!tokens) return NULL;

    if(is_operator(tokens[0]) && !match_lparen(tokens[0].type)){
        fprintf(stderr, "syntax error: exepted string\n");
        return NULL;
    }

    ASTNode *root = parse_seq(tokens, &index);
    if(!root) return NULL;

	if(!match(tokens[index], TOKEN_EOF)){
		fprintf(stderr, "parse: unexepted token '%s'\n", tokens[index].op);
		free_ast(root);
		return NULL;
	}
	return root;
}

// Parsing ; and & - with the lowest priotity
ASTNode *parse_seq(Token *tokens, size_t *index){
	ASTNode *left = parse_logic(tokens, index);
    if(!left) return NULL;

	while(!match(tokens[*index], TOKEN_EOF) && (match(tokens[*index], TOKEN_SEMICOLON) || match(tokens[*index], TOKEN_BACK))){
		NodeType nt = match(tokens[*index], TOKEN_SEMICOLON) ? NODE_SEQ : NODE_BACK;

		++*index;
	//	if(match(tokens[*index], TOKEN_EOF)){
	//		break;
	//	}
		ASTNode *right = parse_logic(tokens, index);
		left = create_binary(nt, left, right);
	}
	
	return left;
}

// Parsing || and &&
ASTNode *parse_logic(Token *tokens, size_t *index){
	ASTNode *left = parse_pipe(tokens, index);
    if(!left) return NULL;
	
	while(!match(tokens[*index], TOKEN_EOF) && (match(tokens[*index], TOKEN_AND) || match(tokens[*index], TOKEN_OR))){
        NodeType nt = match(tokens[*index], TOKEN_AND) ? NODE_AND : NODE_OR;
		char *op = (nt == NODE_AND) ? "&&" : "||";
        ++*index;
		ASTNode *right = parse_pipe(tokens, index);
		
        if(!right){
            if(!(match(tokens[*index], TOKEN_WORD) || match_lparen(tokens[*index].type)))
                fprintf(stderr, "syntax error: expected command after '%s'\n", op);
            free_ast(left);
            return NULL;
        }
        left = create_binary(nt, left, right);
	}

	return left;
}

// Parsing |
ASTNode *parse_pipe(Token *tokens, size_t *index){
	ASTNode *left = parse_group(tokens, index);
	if(!left) return NULL;

	while(!match(tokens[*index], TOKEN_EOF) && (match(tokens[*index], TOKEN_BAR) || match(tokens[*index], TOKEN_BAR_AMP))){
        int pipe_stderr = match(tokens[*index], TOKEN_BAR_AMP);
		++*index;
		ASTNode *right = parse_group(tokens, index);
        if(!right){
            if(!(match(tokens[*index], TOKEN_WORD) || match_lparen(tokens[*index].type)))
                fprintf(stderr, "syntax error: expected command after '|'\n");
            free_ast(left);
            return NULL;
 
        }
		left = create_binary(NODE_PIPE, left, right);

		if(pipe_stderr && left->type == NODE_PIPE && left->binary.left->type == NODE_COMMAND){
			add_redir(&left->binary.left->command.head, OUT_ERR, "1");	
		}
	}
	return left;
}


ASTNode *parse_group(Token *tokens, size_t *index){
	if(match_lparen(tokens[*index].type)){
		TokenType open = tokens[*index].type;
		TokenType close = get_matching_close(open);
		NodeType nd = match(tokens[*index], TOKEN_LPAREN) ? NODE_SUBSHELL : NODE_GROUP;
		
		++*index;
		ASTNode *child = parse_seq(tokens, index);

		if(!match(tokens[*index], close)){
			fprintf(stderr, "Error: expected closing bracket\n");
			return NULL;
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
				free(argv);
				free(redirlist);
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

	if(argc == 0 && redirlist){
		argv = malloc(sizeof(char*));
		argv[0] = NULL;
		return create_command(argc, argv, redirlist);
	}
	if(argc == 0) return NULL;
	return create_command(argc, argv, redirlist);
}


