// exec.c

#include "jobs.h"
#include "func.h"
#include "exec.h"
#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

Function funcs[] = {
	{"echo", echo},
	{"cd", cd},
	{"touch", touch},
	{"mkdir", makedir},
	{"pwd", pwd},
	{"mv", mv},
	{"jobs", jobs}
};
// cd pwd help history exit echo
// jobs fg bg kill

void signal_setter(){
	setpgid(0, 0);
	signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
}

int execute_redirect(Redir *redirs){
	if(!redirs) return 0;
	Redir *head = redirs;
	while(head){
		int file;
		int pipefd[2];
		int fd;
		switch(head->rt){
			case(IN_BUF):
				if(pipe(pipefd) == -1){
					perror("redirect: pipe");
					return 1;
				}		
				
				write(pipefd[1], head->file, strlen(head->file));
				write(pipefd[1], "\n", 1);
				close(pipefd[1]);
		
				if(dup2(pipefd[0], STDIN_FILENO) < 0){
					perror("redirect: dup2");
					close(pipefd[0]);
					return 1;
				}
				close(pipefd[0]);
				break;			
			case(IN_END):
				if(pipe(pipefd) == -1){
					perror("redirect: pipe");
					return 1;
				}		
				
				write(pipefd[1], head->file, strlen(head->file));
				close(pipefd[1]);
		
				if(dup2(pipefd[0], STDIN_FILENO) < 0){
					perror("redirect: dup2");
					close(pipefd[0]);
					return 1;
				}
				close(pipefd[0]);
				break;
			case(IN_ERR):
				fd = atoi(head->file);
				if(dup2(fd, STDIN_FILENO) < 0){
					perror("redirect: dup2");
					return 1;
				}
				break;
			case(IN_NEW):
				file = open(head->file, O_CREAT | O_RDWR, 0666);
				if(file < 0){
					perror("redirect: open");
					return 1;
				}
				if(dup2(file, STDIN_FILENO) < 0){
					perror("cringe");
					close(file);
					return 1;
				}
				close(file);
				break;
			case(IN_OUT):
				file = open(head->file, O_RDONLY);
				if(file < 0){
					perror("redirect: open");
					return 1;
				}
				if(dup2(file, STDIN_FILENO) < 0){
					perror("redirect: dup2");
					close(file);
					return 1;	
				}
				close(file);
				break;
			case(OUT_END):	
				file = open(head->file, O_CREAT | O_WRONLY | O_APPEND, 0666);
				if(file < 0){
					perror("redirect: open");
					return 1;
				}
				if(dup2(file, STDOUT_FILENO) < 0){
					perror("redirect: dup2");
					close(file);
					return 1;
				}
				close(file);
				break;
			case(OUT_ERR):
				fd = atoi(head->file);
				if(dup2(fd, STDOUT_FILENO) < 0){
					perror("redirect: dup2");
					return 1;
				}
				break;
			case(OUT_NEW):
				file = open(head->file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
				if(file < 0){
					perror("redirect: open");
					return 1;
				}
				if(dup2(file, STDOUT_FILENO) < 0){
					perror("redirect: open");
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

int execute_command(ASTNode *root, int back){	
	if(!root) return 1;
	if(root->command.argv){
		int builtin = execute_builtin(root);
		if(builtin >= 0) return builtin;
	}

	pid_t pid = fork();
	if(pid < 0) return 1;
	
	int shellpgid = getpgid(0);
	if(pid == 0){
		signal_setter();

		execute_redirect(root->command.head);
		execvp(root->command.argv[0], root->command.argv);
		perror("execvpe: execute_command:");
		exit(1);
	}

	setpgid(pid, pid);

	if(back) add_job(pid, root->command.argv[0], RUNNING);
	else{
		tcsetpgrp(STDIN_FILENO, pid);
		
		int status;
		waitpid(pid, &status, WUNTRACED);

		tcsetpgrp(STDIN_FILENO, shellpgid);
		return WEXITSTATUS(status);
	}
	return 0;
}

int execute_pipe(ASTNode *root, int back){
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
			execute_command(root->binary.left, 1);
			if(execute_command(root->binary.right, 0)){
			}
			return 0;
		case(NODE_PIPE):
			return execute_pipe(root, 0);
		case(NODE_SUBSHELL):
			pid_t pid = fork();
			if(pid < 0) return 1;
			
			if(pid == 0) exit(execute(root->unary.child));
			
			int status;
			wait(&status);
			return WEXITSTATUS(status);
		case(NODE_COMMAND):
			return execute_command(root, 0);
		default:
			return 1;
	}
}

