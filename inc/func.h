#pragma once
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>

typedef struct Function{
	const char name[128];
	int (*func)(int argc, char *argv[]);
}Function;


int echo(int, char*[]);
int cd(int, char*[]);
int ls(int, char*[]);
int touch(int, char*[]);
int makedir(int, char*[]);
int grep(int, char*[]);
int pwd(int, char*[]);
int mv(int, char*[]);
int cat(int, char*[]);
int head(int, char*[]);
int tail(int, char*[]);
int clear(int, char*[]);
int cp(int, char*[]);


