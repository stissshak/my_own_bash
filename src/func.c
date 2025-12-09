// func.c

#include "func.h"
#include "history.h"
#include "terminal.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int echo(int argc, char *argv[]){
	bool new_line = true, space = true;
	if(argc == 1){
		fprintf(stderr, "Inncorect amount of arguments");
		return 1;
	}
	int i = 1;
	while(i < argc){
		switch(argv[i][0]){
			case('\"'):
				write(STDOUT_FILENO, argv[i] + 1, strlen(argv[i]) - 2);
				break;
			case('-'):
				switch(argv[i][1]){
					case('n'):
						new_line = false;
						break;
					case('s'):
						space = false;
						break;
					default:
						break;
				}
				++i;
				continue;
				break;
			default:
				write(STDOUT_FILENO, argv[i], strlen(argv[i]));
				break;

		}
		++i;	
		if(space) write(STDOUT_FILENO, " ", 1);
	}
	if(new_line) putchar('\n');
	return 0;
}

int cd(int argc, char *argv[]){
	if(argc == 1){
		char home[128] = "/home/";
		uid_t euid = geteuid();
		struct passwd *pw = getpwuid(euid);
		strcat(home, pw->pw_name);

		chdir(home);
		return 0;
	}
	if(argc == 2){
		chdir(argv[1]);
		return 0;
	}
	return 1;
}

int pwd(int argc, char *argv[]){
	(void)argc; (void)argv;
	char pwd[256];
	getcwd(pwd, 256);
	printf("%s\n", pwd);
	return 0;
}

int bexit(int argc, char *argv[]){
	int code = 0;
	if(argc > 1){
		code = atoi(argv[1]);
	}

	history_free();
	disable_raw_mode();
	exit(code);
	return code;
}

int help(int argc, char *argv[]){
	(void)argc; (void)argv;
	printf("sash - stissshak's shell\n\n");
        printf("Built-in commands:\n");
        printf("  cd [dir]    - change directory\n");
        printf("  pwd         - print working directory\n");
        printf("  echo [args] - print arguments\n");
        printf("  history     - show command history\n");
        printf("  jobs        - list background jobs\n");
        printf("  fg [id]     - bring job to foreground\n");
        printf("  bg [id]     - continue job in background\n");
        printf("  exit [code] - exit shell\n");
        printf("  help        - show this help\n");
        return 0;
}
