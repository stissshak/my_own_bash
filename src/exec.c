// exec.c

#define _POSIX_C_SOURCE 200809L


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
	{"pwd", pwd},
	{"jobs", jobs},
	{"fg", fg},
	{"bg", bg},
	{"exit", bexit}
};
// cd pwd help history exit echo
// jobs fg bg kill

void signal_setter(){
	setpgid(0, 0);

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTSTP);
	sigaddset(&mask, SIGTTIN);
	sigaddset(&mask, SIGTTOU);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);

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

int execute_command(ASTNode *root){	
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

	tcsetpgrp(STDIN_FILENO, pid);
		
	int status;
	waitpid(pid, &status, WUNTRACED);
	tcsetpgrp(STDIN_FILENO, shellpgid);

	if(WIFSTOPPED(status)){
		int id = add_job(pid, root->command.argv[0], STOPPED);
		printf("\n[%d] Stopped\t%s\n", id, root->command.argv[0]);
		return 128 + WSTOPSIG(status);
	}

	return WEXITSTATUS(status);
}

int execute_back(ASTNode *root){
	pid_t pid = fork();
	if(pid < 0) return 1;

	if(pid == 0){
		signal_setter();
		int status = execute(root);
		exit(status);	
	}

	setpgid(pid, pid);
	add_job(pid, "background", RUNNING);
	return 0;
}

static int flatten_pipe(ASTNode *root, ASTNode **arr, int max){
	if(!root) return 0;
	if(root->type != NODE_PIPE){
		arr[0] = root;
		return 1;
	}
	int left = flatten_pipe(root->binary.left, arr, max);
	int right = flatten_pipe(root->binary.right, arr + left, max - left);
	return left + right;
}

int execute_pipe(ASTNode *root){
	ASTNode *arr[64];
	int n = flatten_pipe(root, arr, 64);
	if(n < 2) return execute(arr[0]);

	int pipes[n-1][2];
	for(int i = 0; i < n-1; ++i){
		if(pipe(pipes[i]) < 0){
			perror("execute_pipe: pipe");
			for(int j = 0; j < i; ++j){
				close(pipes[j][0]);
				close(pipes[j][1]);
			}
			return 1;
		}
	}

	int shellpgid = getpgid(0);
	pid_t pgid = 0;

	pid_t pids[n];
	for(int i = 0; i < n; ++i){
		pids[i] = fork();
		if(pids[i] < 0){
			perror("execute_pipe: fork");
			return 1;
		}
		if(pids[i] == 0){
			setpgid(0, pgid);
			signal_setter();
			if(i > 0){
				dup2(pipes[i-1][0], STDIN_FILENO);
			}
			if(i < n-1){
				dup2(pipes[i][1], STDOUT_FILENO);
			}

			for(int j = 0; j < n-1; ++j){
				close(pipes[j][0]);
				close(pipes[j][1]);
			}

			if(arr[i]->type == NODE_COMMAND){
				execute_redirect(arr[i]->command.head);
				for(size_t j = 0; j < sizeof(funcs)/sizeof(Function); ++j){
					if(!strcmp(arr[i]->command.argv[0], funcs[j].name)){
						exit(funcs[j].func(arr[i]->command.argc, arr[i]->command.argv));
					}
				}


				execvp(arr[i]->command.argv[0], arr[i]->command.argv);
				perror("execute_pipe: exec");
				exit(127);
			}

			exit(execute(arr[i]));
		}
		if(i == 0) pgid = pids[0];
		setpgid(pids[i], pgid);
	}

	tcsetpgrp(STDIN_FILENO, pgid);

	for(int i = 0; i < n-1; ++i){
		close(pipes[i][0]);
		close(pipes[i][1]);
	}

	int status;
	for(int i = 0; i < n; ++i){
		waitpid(pids[i], &status, 0);
	}

	tcsetpgrp(STDIN_FILENO, shellpgid);

	if(WIFEXITED(status))
		return WEXITSTATUS(status);
	if(WIFSIGNALED(status))
		return 128 + WTERMSIG(status);
	return 1;
}

// Main function that call all other functions
int execute(ASTNode *root){
	if(!root) return 1;
	int status;
	switch(root->type){
		case(NODE_SEQ):
			execute(root->binary.left);
			return execute(root->binary.right);
		case(NODE_AND):
			if(!execute(root->binary.left))
			  	return execute(root->binary.right);
			return 1;
		case(NODE_OR):
			status = execute(root->binary.left);
			if(status != 0) return execute(root->binary.right);
			return status;
		case(NODE_BACK):
			execute_back(root->binary.left);
			if(root->binary.right){
				return execute(root->binary.right);
			}
			return 0;
		case(NODE_PIPE):
			return execute_pipe(root);
		case(NODE_SUBSHELL):
			pid_t pid = fork();
			if(pid < 0) return 1;
			
			if(pid == 0){
				signal_setter();
				exit(execute(root->unary.child));
			}
			
			wait(&status);
			return WEXITSTATUS(status);
		case(NODE_GROUP):
			return execute(root->unary.child);
		case(NODE_COMMAND):
			return execute_command(root);
		default:
			return 1;
	}
}

