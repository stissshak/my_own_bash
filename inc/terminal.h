// terminal.h

#pragma once

#include "jobs.h"
#include <sys/types.h>
#include <signal.h>

extern volatile sig_atomic_t j_upt_need;

typedef enum {
	// MAIN
        KEY_CHAR,
        KEY_ESC,
	// ARROWS
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
	// CLEAN
        KEY_BACKSPACE,
        KEY_DEL,
	// CONTROL
	KEY_ENTER,
	KEY_TAB,
	// CTRL+	
	KEY_CTRL_A,
        KEY_CTRL_E,
        KEY_CTRL_U,
        KEY_CTRL_K,
	KEY_CTRL_D,
	KEY_CTRL_C,
	KEY_CTRL_Z,
	KEY_CTRL_L,
	// EMPTY
        KEY_NONE
} KeyType;

void enable_raw_mode();
void disable_raw_mode();
void set_prompt_func(void (*f)(void));
KeyType read_key(char *);
size_t get_line(char **);
