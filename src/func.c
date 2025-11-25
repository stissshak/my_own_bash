// func.c

#include "func.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

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
				write(STDOUT_FILENO, argv[i] + 1, strlen(argv[1]) - 2);
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
/*
int ls(int argc, const char *argv[]){
	return 0;
}
*/
int touch(int argc, char *argv[]){
	if(argc == 1){
		printf("touch: missing file operand\nTry 'touch --help' for more information.\n");
		return 1;
	}
	int i = 1;
	while(i < argc){
		int fd = creat(argv[i], 0666);
		if(fd < 0){
			printf("touch: can't create file %s\n", argv[i]);
			return 1;
		}
		++i;
	}
	return 0;
}

int makedir(int argc, char *argv[]){
	if(argc == 1){
		printf("mkdir: missing operand\n");
		return 1;	
	}
	int result = 0, i = 1;
	struct stat st = {0};
	while(i < argc){
		if(stat(argv[i], &st) == -1){
			mkdir(argv[i], 0700);
		}
		else{
			printf("mkdir: cannot create directory %s: File exists\n", argv[i]);
			result = 1;
		}
		++i;
	}
	return result;
}

/*
int grep(int argc, const char *argv[]){

	return 0;
}
*/
int pwd(int argc, char *argv[]){
	char pwd[256];
	getcwd(pwd, 256);
	printf("%s\n", pwd);
	return 0;
}

int mv(int argc, char *argv[]){
	if(argc == 1){
		printf("mv: missing file operand\n");
		return 1;
	}
	if(argc == 2){
		printf("mv: missing destination file operand after \'%s\'\n", argv[1]);
		return 1;
	}
	return rename(argv[1], argv[2]);
}

int cat(int argc, char *argv[]){
	char buf[256];
	if(argc == 1){
		ssize_t sread;
		while(sread = read(STDIN_FILENO, buf, 256)){
			write(STDOUT_FILENO, buf, sread);
		}
		return 0;
	}
	if(argc == 2){
		int fd = open(argv[1], O_RDONLY);
		ssize_t sread;
		while(sread = read(fd, buf, 256)){
			write(STDOUT_FILENO, buf, sread);
		}
		return 0;
	}

	return 0;
}

/*
int head(int argc, const char *argv[]){

	return 0;
}

int tail(int argc, const char *argv[]){

	return 0;
}


int clear(int argc, const char *argv[]){

	return 0;
}

int cp(int argc, const char *argv[]){
	return 0;
}
*/
