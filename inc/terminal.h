// terminal.h

#pragma once

#include "jobs.h"
#include <sys/types.h>
#include <signal.h>

extern volatile sig_atomic_t j_upt_need;

typedef enum {
        KEY_CHAR,
        KEY_ESC,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_BACKSPACE,
        KEY_ENTER,
	KEY_CTRL_D,
	KEY_CTRL_C,
	KEY_CTRL_Z,
	KEY_CTRL_L,
        KEY_NONE
} KeyType;

void enable_raw_mode();
void disable_raw_mode();
void set_prompt_func(void (*f)(void));
KeyType read_key(char *);
size_t get_line(char **);
