// exec.c

#define _POSIX_C_SOURCE 200809L

#include "jobs.h"
#include "history.h"
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
	{"history", history},
	{"jobs", jobs},
	{"fg", fg},
	{"bg", bg},
	{"kill", bkill},
	{"exit", bexit},
	{"help", help},
};

static const size_t num_funcs = sizeof(funcs) / sizeof(funcs[0]);
static int bg_m = 0;

void signal_setter(){
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
	signal(SIGTERM, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
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
			case(OUT_ALL_END):
				file = open(head->file, O_CREAT | O_WRONLY | O_APPEND, 0666);
				if(file < 0){
					perror("redirect: open");
					return 1;
				}
				if(dup2(file, STDOUT_FILENO) < 0){
					perror("redirect: open");
					close(file);
					return 1;
				}
				if(dup2(file, STDERR_FILENO) < 0){
					perror("redirect: open");
					close(file);
					return 1;
				}
				close(file);
				break;
			case(OUT_ALL):
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
				if(dup2(file, STDERR_FILENO) < 0){
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

static int (*get_builtin(char *name))(int, char **){
	for(size_t i = 0; i < num_funcs; ++i){
		if(!strcmp(name, funcs[i].name)) return funcs[i].func;
	}
	return NULL;
}

int execute_builtin(ASTNode *root){
	int (*func)(int, char**) = get_builtin(root->command.argv[0]);
	if(!func) return -1;
	int saved_stdin = dup(STDIN_FILENO);	
	int saved_stdout = dup(STDOUT_FILENO);	
	int saved_stderr = dup(STDERR_FILENO);	
				
	execute_redirect(root->command.head);
	int status = func(root->command.argc, root->command.argv);
			
	dup2(saved_stdin, STDIN_FILENO);	
	dup2(saved_stdout, STDOUT_FILENO);	
	dup2(saved_stderr, STDERR_FILENO);

	close(saved_stdin);	
	close(saved_stdout);	
	close(saved_stderr);
			
	return status;	
}

static void exec_child(ASTNode *root){
	execute_redirect(root->command.head);
	
	int (*func)(int, char**) = get_builtin(root->command.argv[0]);
	if(func) exit(func(root->command.argc, root->command.argv));

	execvp(root->command.argv[0], root->command.argv);
	perror(root->command.argv[0]);
	exit(1);
}

int execute_command(ASTNode *root){	
	if(!root) return 1;
	if(!root->command.argv) return 0;

	if(!bg_m){
		int builtin = execute_builtin(root);
		if(builtin >= 0) return builtin;
	}

	pid_t pid = fork();
	if(pid < 0) return 1;
	
	if(pid == 0){
		setpgid(0, 0);
		signal_setter();
		exec_child(root);
	}

	setpgid(pid, pid);
	
	if(bg_m){
		char *name = ast_to_string(root);
		pid_t pids[] = {pid};
		int id = add_job(create_job(pid, pids, 1, name, RUNNING));
		printf("[%d] %d\n", id, pid);
		free(name);
		return 0;
	}

	tcsetpgrp(STDIN_FILENO, pid);
	
	int status;
	waitpid(pid, &status, WUNTRACED);

	tcsetpgrp(STDIN_FILENO, getpgrp());
	
	if(WIFSTOPPED(status)){
		char *name = ast_to_string(root);
		pid_t pids[] = {pid};
		int id = add_job(create_job(pid, pids, 1, name, STOPPED));
		printf("\n[%d] Stopped\t%s\n", id, root->command.argv[0]);
		free(name);
		return 128 + WSTOPSIG(status);
	}
	if(WIFEXITED(status)) return WEXITSTATUS(status);
	if(WIFSIGNALED(status)) return 128 + WTERMSIG(status);
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

	pid_t pids[n];
	pid_t pgid = 0;
	int prev_fd = -1;
	int fd[2];

	for(int i = 0; i < n; ++i){
		int is_last = (i == n-1);
		
		if(!is_last && pipe(fd) < 0){
			perror("execute_pipe: pipe");
			if(prev_fd >= 0) close(prev_fd);
			for(int j = 0; j < i; ++j){
				kill(pids[j], SIGTERM);
				waitpid(pids[j], NULL, 0);
			}
			return 1;
		}

		pids[i] = fork();
		
		if(pids[i] < 0){
			perror("execute_pipe: fork");
			if(prev_fd >= 0) close(prev_fd);
			if(!is_last){
				close(fd[0]); close(fd[1]);
			}
			for(int j = 0; j < i; ++j){
				kill(pids[j], SIGTERM);
				waitpid(pids[j], NULL, 0);
			}
			return 1;
		}

		if(pids[i] == 0){
			setpgid(0, pgid);
			signal_setter();
			
			if(prev_fd >= 0){
				dup2(prev_fd, STDIN_FILENO);
				close(prev_fd);
			}
			
			if(!is_last){
				dup2(fd[1], STDOUT_FILENO);
				close(fd[0]);
				close(fd[1]);
			}
			
			if(arr[i]->type == NODE_COMMAND) exec_child(arr[i]);
			exit(execute(arr[i]));
		}
		if(i == 0) pgid = pids[0];
		setpgid(pids[i], pgid);

		if(prev_fd >= 0) close(prev_fd);
		if(!is_last){
			prev_fd = fd[0];
			close(fd[1]);
		}
	}

    char *name = ast_to_string(root);	
	job_t *job = create_job(pgid, pids, n, name, RUNNING);
	free(name);

	if(bg_m){    
        int id = add_job(job);
        printf("[%d] %d\n", id, pgid);
        return 0;
    }

	tcsetpgrp(STDIN_FILENO, pgid);

	int status;
	int stopped = 0;
	for(int i = 0; i < n; ++i){
		waitpid(pids[i], &status, WUNTRACED);
		if(WIFSTOPPED(status)){
            job->pids[i].status = STOPPED;
            stopped = 1;
        }
	}

	tcsetpgrp(STDIN_FILENO, getpgrp());

	if(stopped){
		char *name = ast_to_string(root);
        job->status = STOPPED;
		int id = add_job(job);
		printf("\n[%d] Stopped\t%s\n", id, name);
		free(name);
		return 128 + WSTOPSIG(status);
	}
    free(job);
	if(WIFEXITED(status)) return WEXITSTATUS(status);
	if(WIFSIGNALED(status)) return 128 + WTERMSIG(status);
	return 0;
}

int execute_subshell(ASTNode *root){
	pid_t pid = fork();
	if(pid < 0) return 1;

	if(pid == 0){
		setpgid(0, 0);
		signal_setter();
		exit(execute(root->unary.child));
	}

	setpgid(pid, pid);

	if(bg_m){
		char *name = ast_to_string(root);
		pid_t pids[] = {pid};
		int id = add_job(create_job(pid, pids, 1, name, RUNNING));
		printf("[%d] %d\n", id, pid);
		free(name);
		return 0;
	}

	tcsetpgrp(STDIN_FILENO, pid);
	
	int status;
	waitpid(pid, &status, WUNTRACED);

	tcsetpgrp(STDIN_FILENO, getpgrp());
	
	if(WIFSTOPPED(status)){
		char *name = ast_to_string(root);
		pid_t pids[] = {pid};
		int id = add_job(create_job(pid, pids, 1, name, STOPPED));
		printf("\n[%d] Stopped\t%s\n", id, name);
		free(name);
		return 128 + WSTOPSIG(status);
	}
	if(WIFEXITED(status)) return WEXITSTATUS(status);
	if(WIFSIGNALED(status)) return 128 + WTERMSIG(status);
	return 0;
}

// Main function that call all other functions
int execute(ASTNode *root){
	if(!root) return 0;
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
			bg_m = 1;
			execute(root->binary.left);
			bg_m = 0;
			if(root->binary.right){
				return execute(root->binary.right);
			}
			return 0;
		case(NODE_PIPE):
			return execute_pipe(root);
		case(NODE_SUBSHELL):
			return execute_subshell(root);	
		case(NODE_GROUP):
			return execute(root->unary.child);
		case(NODE_COMMAND):
			return execute_command(root);
		default:
			return 1;
	}
}


