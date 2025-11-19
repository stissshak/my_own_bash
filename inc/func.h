#pragma once
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>

typedef struct Function{
	const char name[128];
	int (*func)(int argc, const char *argv[]);
}Function;

int echo(int, const char*[]);
int cd(int, const char*[]);
int ls(int, const char*[]);
int touch(int, const char*[]);
int mkdir(int, const char*[]);
int grep(int, const char*[]);
int pwd(int, const char*[]);
int mv(int, const char*[]);
int cat(int, const char*[]);
int head(int, const char*[]);
int tail(int, const char*[]);
int clear(int, const char*[]);
int cp(int, const char*[]);

Function funcs[] = {
	{"echo", echo},
	{"cd", cd}
};

int echo(int argc, const char *argv[]){
	if(argc < 2 || argc > 3){
		fprintf(stderr, "Inncorect amount of arguments");
		return 1;
	}
	write(STDOUT_FILENO, argv[1], strlen(argv[1]));
	return 0;
}

int cd(int argc, const char *argv[]){
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

int ls(int argc, const char *argv[]){

	return 0;
}

int touch(int argc, const char *argv[]){

	return 0;
}

int mkdir(int argc, const char *argv[]){

	return 0;
}

int grep(int argc, const char *argv[]){

	return 0;
}

int pwd(int argc, const char *argv[]){

	return 0;
}

int mv(int argc, const char *argv[]){

	return 0;
}

int cat(int argc, const char *argv[]){

	return 0;
}

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








	
