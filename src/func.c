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

#define _POSIX_C_SOURCE 200809L

int echo(int argc, char *argv[]){
	bool new_line = true, space = true;
	if(argc == 1){
		printf("\n");
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
    char oldpwd[256];
    getcwd(oldpwd, sizeof(oldpwd));

	char *path;
    if(argc < 2 || !argv[1]) path = getenv("HOME");
    else if(!strcmp(argv[1], "-")) path = getenv("OLDPWD");
    else path = argv[1];
    
    if(chdir(path) != 0){
        perror("cd");
        return 1;
    }

    setenv("OLDPWD", oldpwd, 1);

    char newpwd[256];
    getcwd(newpwd, sizeof(newpwd));
    setenv("PWD", newpwd, 1);
	return 0;
}

int pwd(int argc, char *argv[]){
	(void)argc; (void)argv;
	char pwd[256];
	getcwd(pwd, sizeof(pwd));
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
