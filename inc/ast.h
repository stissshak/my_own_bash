// ast.h

#pragma once

#include "token.h"

//////////////////////////////

typedef enum {
	NODE_COMMAND,	// ls, cat, ...
	NODE_PIPE,	// cmd1 | cmd2
	NODE_SEQ,	// cmd1 ; cmd2
	NODE_AND,	// cmd1 && cmd2
	NODE_OR,	// cmd1 || cmd2
	NODE_BACK,	// cmd1 &
	NODE_SUBSHELL,	// (...)
	NODE_GROUP	// {...}
} NodeType;

typedef enum {
	IN_BUF,		// <<< 
	IN_END,		// <<
	IN_ERR,		// <&
	IN_NEW,		// <>
	IN_OUT,		// <
	OUT_END,	// >>
	OUT_ERR,	// >&
	OUT_NEW,	// >
	OUT_ALL_END, 	// &>>
	OUT_ALL, 	// &>
} RedirType;

typedef struct Redir{
	RedirType rt;
	char *file;
	struct Redir *next;
} Redir;

typedef struct ASTNode {
	NodeType type;
	union {
		struct {
			char **argv;
			Redir *head;
			int argc;
		} command;

		struct {
			struct ASTNode *left, *right;
		} binary;
		
		struct {
			struct ASTNode *child;
		} unary;
	};
} ASTNode;

////////////////////////////////

ASTNode *create_node(NodeType);
ASTNode *create_command(int, char**, Redir*);
ASTNode *create_binary(NodeType, ASTNode*, ASTNode*);
ASTNode *create_unary(NodeType, ASTNode*);
void add_redir(Redir**, RedirType, const char*); 
void free_ast(ASTNode*);

void print_level(int);
const char *get_node_name(NodeType);
const char *get_redir_name(RedirType);
void print_tree(ASTNode*, int);
void print_ast(ASTNode*);

///////////////////////////////


