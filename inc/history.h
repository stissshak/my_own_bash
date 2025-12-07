// history.h

#pragma once

#include <stddef.h>

#define MAX_HISTORY 128

typedef struct HistoryList{
	char *cmd;
	struct HistoryList *next, *prev;
}HistoryList;	

void history_init();
void history_free();
void history_add(const char*);
const char *history_get(size_t);


void history_reset_cursor();
const char *history_prev();
const char *history_next();

int history(int, char*[]);


