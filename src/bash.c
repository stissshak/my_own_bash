#define _DEFAULT_SOURCE

#include "lexer.h"
#include "parser.h"
#include "exec.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>

#define PRINT_TIME 30000000
#define BUF_SIZE 1024
#define DSS 128


void greeting(){
	struct timespec tv = {.tv_sec = 0, .tv_nsec = PRINT_TIME};
	char greeting[] = "Welcome to stissshak's bash - sash!\nMy bash is not that big like fish or zsh, but it's my project\n";
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

	char home[] = "/home/";
	uid_t euid = geteuid();
	struct passwd *pw = getpwuid(euid);

	strcat(home, pw->pw_name);
	if(!strncmp(home, pwd, strlen(home))){
		buf[0] = '~';
		strcpy(buf + 1, pwd + strlen(home));
		strcpy(pwd, buf);
	}

	printf("%s@%s %s> ", pw->pw_name, host, pwd);
	fflush(stdout);
}

int main(){
	greeting();
	char buf[BUF_SIZE];
	while(1){
		path();
		int len = read(STDIN_FILENO, buf, BUF_SIZE);
		if(len == 0){
			putchar('\n');
			exit(0);
		}	
		buf[len-1] = '\0';
		Token *tokens = tokenize(buf);
		ASTNode *root = parse(tokens);
		execute(root);
		clean_tokens(tokens);
		free_ast(root);
	}
	return 0;
}
