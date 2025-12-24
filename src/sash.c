// bash.c
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include "lexer.h"
#include "parser.h"
#include "exec.h"
#include "jobs.h"
#include "terminal.h"
#include "history.h"
#include "colors.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>

#define PRINT_TIME 30000000
#define BUF_SIZE 1024
#define DSS 128

volatile sig_atomic_t j_upt_need = 0;

void signal_handler(int sig){
	(void)sig;
	j_upt_need = 1;
}

void greeting(){
	struct timespec tv = {.tv_sec = 0, .tv_nsec = PRINT_TIME};
	char greeting[] = "Welcome to stissshak's bash - "
		COLOR_BOLD_CYAN"sash!\n"COLOR_RESET
		"My bash is not that big like "
		COLOR_BOLD_GREEN"fish"COLOR_RESET
		" or "
		COLOR_BOLD_RED"zsh,"COLOR_RESET 
		" but it's "
		COLOR_BOLD_CYAN"my project\n"COLOR_RESET;
	for(size_t i = 0; i < sizeof(greeting); ++i){
		putchar(greeting[i]);
		nanosleep(&tv, NULL);
		tv.tv_nsec = PRINT_TIME;
		fflush(stdout);
	}
}

void path(){
	char buf[DSS];
	char pwd[DSS];
	getcwd(pwd, DSS);	
	char host[DSS];
	gethostname(host, DSS);

	char home[DSS] = "/home/";
	uid_t euid = geteuid();
	struct passwd *pw = getpwuid(euid);

	strcat(home, pw->pw_name);
	if(!strncmp(home, pwd, strlen(home))){
		buf[0] = '~';
		strcpy(buf + 1, pwd + strlen(home));
		strcpy(pwd, buf);
	}

	printf(COLOR_GREEN"%s"COLOR_RESET"@%s "COLOR_GREEN"%s"COLOR_RESET"> ", pw->pw_name, host, pwd);
	fflush(stdout);
}

void signals(){
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sa.sa_flags = SA_NOCLDSTOP;
	sigemptyset(&sa.sa_mask);
	if(sigaction(SIGCHLD, &sa, NULL) == -1){
		perror("signals:");
	}
}

int main(){
	greeting();
	signals();
	history_init();
	
	set_prompt_func(path);

	char *buf;
	while(1){
		enable_raw_mode();
		path();
		int len = get_line(&buf);
		
		if(j_upt_need){
			update_jobs();
			j_upt_need = 0;
		}
	
		if(buf == NULL){
			putchar('\n');
			break;
		}

		if(len == 0){
			free(buf);
			continue;
		}

		history_add(buf);
        Token *tokens = tokenize(buf);
		ASTNode *root = parse(tokens);
		disable_raw_mode();
		execute(root);
		clean_tokens(tokens);
		free_ast(root);
		free(buf);
	}
	history_free();
	return 0;
}
