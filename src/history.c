// history.c

#include "history.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>

#define DSS 128 // DEFAULT STRING SIZE

static HistoryList *head, *end, *cursor;
static size_t count = 0;
int fd;

static char *extract_command(){
	int size = DSS, counter = 0;
	char *buf = malloc(size);
	if(!buf){
		perror("extract_word");
		return NULL;
	}
	bool s1 = false, s2 = false; // s1 - flag that \" is open, s2 - \'
	char c;
	while(1){
		if(counter == size - 1){
			size *= 2;
			buf = realloc(buf, size);
			if(!buf){
				perror("extrac_word");
				return NULL;
			}
		}
		if(read(fd, &c, 1) <= 0) break;
		if(c == '\"') s1 = !s1;
		else if(c == '\'') s2 = !s2;
		if(!s1 && !s2 && c == '\n') break;
		buf[counter++] = c; 
	}
	buf = realloc(buf, counter + 1);
	if(!buf){
		perror("extrac_word");
		return NULL;
	}
	buf[counter] = '\0';
        return buf;	
}

static void extract_commands(){
	head = end = cursor = NULL;

	char *cmd;

	while((cmd = extract_command()) && cmd[0] != '\0'){
		HistoryList *buf = malloc(sizeof(HistoryList));
		if(!buf){
			perror("extrct_commands");
			return;
		}

		buf->cmd = cmd;
		buf->prev = NULL;
		buf->next = head;
		if(head) head->prev = buf;
		else end = buf;

		head = buf;
		++count;
	}
    free(cmd);
	cursor = head;
}

//////////////////////////////////////////

void history_init(){
	char home[DSS] = "/home/";
	uid_t euid = geteuid();
	struct passwd *pw = getpwuid(euid);
	strcat(home, pw->pw_name);
	strcat(home, "/.sash_history");
	fd = open(home, O_CREAT | O_RDWR, 0666);
	extract_commands();
}

void history_free(){
	HistoryList *buf;
	while(head){
		buf = head;
		head = head->next;
		free(buf->cmd);
		free(buf);
	}
	close(fd);
}

static HistoryList *old_command(const char *str){
	HistoryList *find = head;
	while(find){
		if(!strcmp(str, find->cmd)){
			return find;
		}
		find = find->next;
	}
	return find;
}

void history_add(const char* str){
	history_reset_cursor();	
	HistoryList *buf = old_command(str);
	if(buf){
		if(buf == head) return;
		head->prev = buf;
		if(buf->next) buf->next->prev = buf->prev;
		if(buf->prev) buf->prev->next = buf->next;
		buf->next = head;
		buf->prev = NULL;
		head = buf;
		return;
	}
	write(fd, str, strlen(str));
	write(fd, "\n", 1);
	if(!head){
		head = malloc(sizeof(HistoryList));
		if(!head){
			perror("history_add");	
			return;
		}
		head->cmd = strdup(str);
		if(!head->cmd){
			perror("history_add");
			free(head);
			head = NULL;
			return;
		}
		head->next = head->prev = NULL;
		end = head;
		return;
	}
	head->prev = malloc(sizeof(HistoryList));
	if(!head->prev){
		perror("history_add");
		return;
	}
	head->prev->cmd = strdup(str);
	if(!head->prev->cmd){
		free(head->prev);
		perror("history_add");
		return;
	}
	head->prev->next = head;
	head->prev->prev = NULL;
	head = head->prev;
}

void history_reset_cursor(){ cursor = NULL; }

const char *history_prev(){
	if(!cursor) return NULL;
	if(cursor->prev){ 
		cursor = cursor->prev;
		return cursor->cmd;
	}
	cursor = NULL;
	return "\0";
}

const char *history_next(){
	if(!cursor){
		if(!head) return NULL;
		cursor = head;
		return cursor->cmd;
	}
	if(cursor->next) cursor = cursor->next;
	return cursor->cmd;
}
/////////////////////////////////////////

int history(int argc, char *argv[]){
	(void)argc; (void)argv;
	HistoryList *buf = head;
	while(buf){
		printf("%s\n", buf->cmd);
		buf = buf->next;
	}
	return 0;
}
