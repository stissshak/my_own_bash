// ast.c

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>


///////////////////////////////////////////////////

// Basic creating of nodes
ASTNode *create_node(NodeType type){
	ASTNode *node = malloc(sizeof(ASTNode));
	if(!node){
		perror("create_node");
		exit(1);
	}
	node->type = type;
	return node;
}

// We got all parametrs of command, so we can easy create it
ASTNode *create_command(int argc, char** argv, Redir *redir){
	ASTNode *node = create_node(NODE_COMMAND);
	node->command.argv = argv;
	node->command.argc = argc;
	node->command.head = redir;
	return node;
}

ASTNode *create_binary(NodeType type, ASTNode *left, ASTNode *right){
	ASTNode *node = create_node(type);
	node->binary.left = left;
	node->binary.right = right;
	return node;
}

ASTNode *create_unary(NodeType type, ASTNode *child){
	ASTNode *node = create_node(type);
	node->unary.child = child;
	return node;
}

// We add redir to end of RedirList
void add_redir(Redir **head, RedirType rt, const char *file){
	Redir *r = malloc(sizeof(Redir));
	if(!r){
		perror("add_redir");
	}
	r->rt = rt;
	r->file = strdup(file);
	if(!r->file){
		perror("add_redir");
	}
	r->next = NULL;

	Redir *buf = *head;
	if(!buf){
		*head = r;
		return;
	}
	
	while(buf->next){
		buf = buf->next;
	}
	buf->next = r;
}

static char *str_append(char *left, char *right){
	if(!right) return left;
	if(!left) return strdup(right);

	size_t len1 = strlen(left);
	size_t len2 = strlen(right);
	char *new = realloc(left, len1 + len2 + 1);
	if(!new){
		free(left);
		return NULL;
	}
	strcpy(new + len1, right);
	return new;
}

char *ast_to_string(ASTNode *root){
	if(!root) return strdup("");

	char *result = NULL;
	char *left, *right, *child;

	switch(root->type){
		case NODE_COMMAND:
			for(int i = 0; i < root->command.argc; ++i){
				if(i > 0) result = str_append(result, " ");
				result = str_append(result, root->command.argv[i]);
			}
			break;
		case NODE_PIPE:
			left = ast_to_string(root->binary.left);
			right = ast_to_string(root->binary.right);
			result = str_append(result, left);
			result = str_append(result, " | ");
			result = str_append(result, right);
			free(left);
			free(right);
			break;
		case NODE_SEQ:
			left = ast_to_string(root->binary.left);
			right = ast_to_string(root->binary.right);
			result = str_append(result, left);
			result = str_append(result, " ; ");
			result = str_append(result, right);
			free(left);
			free(right);
			break;
			break;
		case NODE_AND:
			left = ast_to_string(root->binary.left);
			right = ast_to_string(root->binary.right);
			result = str_append(result, left);
			result = str_append(result, " && ");
			result = str_append(result, right);
			free(left);
			free(right);
			break;
			break;
		case NODE_OR:
			left = ast_to_string(root->binary.left);
			right = ast_to_string(root->binary.right);
			result = str_append(result, left);
			result = str_append(result, " || ");
			result = str_append(result, right);
			free(left);
			free(right);
			break;
			break;
		case NODE_BACK:
			left = ast_to_string(root->binary.left);
			result = str_append(result, left);
			result = str_append(result, " &");
			free(left);
			break;
		case NODE_SUBSHELL:
			child = ast_to_string(root->unary.child);
			result = str_append(result, "(");
			result = str_append(result, child);
			result = str_append(result, ")");
			free(child);
			break;
		case NODE_GROUP:
			child = ast_to_string(root->unary.child);
			result = str_append(result, "{ ");
			result = str_append(result, child);
			result = str_append(result, " }");
			free(child);	
			break;
		default:
			result = strdup("?");
			break;
	}
	return result;
}

void free_ast(ASTNode *root){
	if(!root) return;
	switch(root->type){
		// We need to clean all in argv and list of redirs
		case NODE_COMMAND:
			for(int i = 0; i < root->command.argc; ++i)
				free(root->command.argv[i]);
			free(root->command.argv);
			
			for(Redir *r = root->command.head; r;){
				Redir *next = r->next;
				free(r->file);
				free(r);
				r = next;
			}
			break;
		case NODE_PIPE:		
		case NODE_SEQ:	
		case NODE_AND:	
		case NODE_OR:
		case NODE_BACK:
			free_ast(root->binary.left);
			free_ast(root->binary.right);
			break;
		case NODE_SUBSHELL:
		case NODE_GROUP:
			free_ast(root->unary.child);
			break;
	}
	free(root);
}

// Funcs to see AST

void print_level(int level){
	for(int i = 0; i < level; i++){
	printf("  ");
	}
}


const char* get_node_name(NodeType type){
	switch(type){
		case NODE_COMMAND: return "COMMAND";
		case NODE_PIPE: return "PIPE";
       		case NODE_SEQ: return "SEQUENCE";
       		case NODE_AND: return "AND";
       		case NODE_OR: return "OR";
       		case NODE_BACK: return "BACKGROUND";
       		case NODE_SUBSHELL: return "SUBSHELL";
		case NODE_GROUP: return "GROUP";
       		default: return "UNKNOWN";
	}
}


const char* get_redir_name(RedirType rt){
	switch(rt){
		case IN_BUF: return "<<<";
		case IN_END: return "<<";
		case IN_ERR: return "<&";
       		case IN_NEW: return "<>";
       		case IN_OUT: return "<";
       		case OUT_END: return ">>";
       		case OUT_ERR: return ">&";
       		case OUT_NEW: return ">";
		case OUT_ALL_END: return "&>>";
		case OUT_ALL: return "&>";
		default: return "?";
	}
}


void print_tree(ASTNode *root, int level){
	if(!root){
		print_level(level);
		printf("(Null)\n");
		return;
	}

	print_level(level);
	printf("[%s]\n", get_node_name(root->type));

	switch(root->type){
		case NODE_COMMAND:
			print_level(level + 1);
			printf("argv: ");
			for(int i = 0; i < root->command.argc; ++i){
				printf("\"%s\"", root->command.argv[i]);
				if(i < root->command.argc - 1) printf(", ");
			}
			putchar('\n');

			if(root->command.head){
				print_level(level + 1);
				printf("redirections:\n");
				for(Redir *r = root->command.head; r; r = r->next){
					print_level(level + 2);
					printf("%s %s \n", get_redir_name(r->rt), r->file);
				}
			}
			break;
		case NODE_PIPE:
	        case NODE_SEQ:
		case NODE_BACK:
	        case NODE_AND:
	        case NODE_OR:
	            print_level(level + 1);
	            printf("left:\n");
	            print_tree(root->binary.left, level + 2);
	
	            print_level(level + 1);
	            printf("right:\n");
	            print_tree(root->binary.right, level + 2);
	            break;
	
	        case NODE_SUBSHELL:
		case NODE_GROUP:
	            print_level(level + 1);
	            printf("child:\n");
	            print_tree(root->unary.child, level + 2);
	            break;

	}


}

void print_ast(ASTNode *root){
	printf("\n============== AST ==============\n");
	print_tree(root, 0);
	printf("=================================\n");

}

