// funcs.h

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
int pwd(int, char*[]);
int bexit(int, char*[]);
