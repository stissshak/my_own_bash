#pragma once
#include "parser.h"
#include "func.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int execute(ASTNode*);
int execute_command(ASTNode *);
int execute_pipe(ASTNode*);
int execute_redirect(Redir *redirs);

/////////////////////////////////

int execute_redirect(Redir *redirs){
	if(!redirs) return 1;
	Redir *head = redirs;
	while(head){
		int file;
		switch(head->rt){
			case(IN_BUF):
									
			case(IN_END):
				
			case(IN_ERR):

			case(IN_NEW):
				file = open(head->file, O_RDONLY, 0666);
				if(dup2(file, STDIN_FILENO) < 0){
					perror("cringe");
					close(file);
					return 1;
				}
				close(file);
				break;

			case(IN_OUT):

			case(OUT_END):
					
				file = open(head->file, O_CREAT | O_WRONLY | O_APPEND, 0666);
				if(dup2(file, STDOUT_FILENO) < 0){
					perror("cringe");
					close(file);
					return 1;
				}
				close(file);
				break;
			case(OUT_ERR):

			case(OUT_NEW):
				file = open(head->file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
				if(dup2(file, STDOUT_FILENO) < 0){
					perror("cringe");
					close(file);
					return 1;
				}
				close(file);
				break;
			default:
				return 1;
		}
		head = head->next;
	}
	return 0;
}

int execute_builtin(ASTNode *root){
	for(size_t i = 0; i < sizeof(funcs)/sizeof(Function); ++i){
		if(!strcmp(root->command.argv[0], funcs[i].name)){
			int saved_stdin = dup(STDIN_FILENO);	
			int saved_stdout = dup(STDOUT_FILENO);	
			int saved_stderr = dup(STDERR_FILENO);	
			
				
			execute_redirect(root->command.head);
			int status = funcs[i].func(root->command.argc, root->command.argv);
			
			dup2(saved_stdin, STDIN_FILENO);	
			dup2(saved_stdout, STDOUT_FILENO);	
			dup2(saved_stderr, STDERR_FILENO);

			close(saved_stdin);	
			close(saved_stdout);	
			close(saved_stderr);
			
			return status;	
		}
	}
	return -1;
}

int execute_command(ASTNode *root){
	
	int builtin = execute_builtin(root);
	if(builtin >= 0) return builtin;
	pid_t pid = fork();
	if(pid < 0) return 1;
	if(pid == 0){
		execute_redirect(root->command.head);	
		int status = execvp(root->command.argv[0], root->command.argv);
		if(status < 0){
			perror("execvpe: execute_command:");
			exit(1);
		}
	}
	int status;
	wait(&status);
	return WEXITSTATUS(status);
}

int execute_pipe(ASTNode *root){
	*root;
	return 0;	
}

int execute_back(ASTNode *root){
	*root;
	return 0;
}

// Main function that call all other functions
int execute(ASTNode *root){
	if(!root) return 1;
	switch(root->type){
		case(NODE_SEQ):
			execute(root->binary.left);
			return execute(root->binary.right);
		case(NODE_AND):
			if(!execute(root->binary.left))
			  	return execute(root->binary.right);
			return 1;
		case(NODE_OR):
			if(execute(root->binary.left))
				execute(root->binary.right);
			return 0;
		case(NODE_BACK):
			return execute_back(root->unary.child);
		case(NODE_PIPE):
			return execute_pipe(root);
		case(NODE_SUBSHELL):
			pid_t pid = fork();
			if(pid < 0) return 1;
			
			if(pid == 0) exit(execute(root->unary.child));
			
			int status;
			wait(&status);
			return WEXITSTATUS(status);
	//	case(NODE_ARITHM):
	//		return evaluate(root->unary.child);
		case(NODE_COMMAND):
			return execute_command(root);
		default:
			return 0;
	}
}
