// terminal.c

#include "terminal.h"
#include "jobs.h"
#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#define DEFAULT_SIZE 256

struct termios oldt, newt;

static void (*reprint_greeting)(void) = NULL; 

void set_prompt_func(void (*f)(void)){
	reprint_greeting = f;
}

void enable_raw_mode() {
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO | ISIG);
	tcsetattr(STDIN_FILENO, TCSADRAIN, &newt);
}

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &oldt);
}

KeyType read_key(char *out_char) {
	char c;

	if (read(STDIN_FILENO, &c, 1) < 1) return KEY_NONE;

	if (c == 4) return KEY_CTRL_D;
	if (c == 3) return KEY_CTRL_C;
	if (c == 26) return KEY_CTRL_Z;
	if (c == 12) return KEY_CTRL_L;

	if (c >= 32 && c <= 126) {
		*out_char = c;
		return KEY_CHAR;
	}

	if (c == '\n' || c == '\r') return KEY_ENTER;

	if (c == 127) return KEY_BACKSPACE;

	if (c == '\x1b') { // ESC
		char seq[2];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

		if (seq[0] == '[') {
			switch (seq[1]) {
				case 'A':
					return KEY_UP;
				case 'B':
					return KEY_DOWN;
				case 'C': 
					return KEY_RIGHT;
				case 'D': 
					return KEY_LEFT;
				default:
					return KEY_ESC;
			}
		}
		return KEY_ESC;
	}
	return KEY_ESC;
}

static void redraw_line(char *buf, size_t len, size_t cur){
	write(STDOUT_FILENO, "\r\x1b[K", 4);
	if (reprint_greeting) reprint_greeting();
 	write(STDOUT_FILENO, buf, len);
    
	if (cur < len) {
		char esc[16];
		int n = snprintf(esc, sizeof(esc), "\x1b[%zuD", len - cur);
		write(STDOUT_FILENO, esc, n);
	}
}

size_t get_line(char **str) {
	size_t n = DEFAULT_SIZE, len = 0, cursor = 0;
	*str = malloc(n);
	if(!*str) return 0;
	char *buf = *str;

	char c;	
	int finished = 0;
	history_reset_cursor();
	while (!finished) {
		KeyType kt = read_key(&c);		
		if (kt == KEY_NONE){
			if (j_upt_need){
				j_upt_need = 0;
				update_jobs();
				redraw_line(buf, len, cursor);
			}
		}

		switch (kt) {
			case KEY_CHAR:
				if (len >= n - 1){
					n *= 2;
					char *new_buf = realloc(buf, n);
					if(!new_buf){
						free(buf);
						*str = NULL;
						return 0;
					}
					buf = new_buf;
					*str = buf;
				}

				for (size_t i = len; i > cursor; --i)
					buf[i] = buf[i - 1];
				buf[cursor] = c;
				cursor++;
				len++;

				write(STDOUT_FILENO, &c, 1);
				write(STDOUT_FILENO, "\x1b[s", 3);
				write(STDOUT_FILENO, buf + cursor, len - cursor);
				write(STDOUT_FILENO, "\x1b[u", 3);
				break;
			case KEY_BACKSPACE:
				if (cursor == 0) break;

				for (size_t i = cursor; i < len; ++i) {
					buf[i - 1] = buf[i];
				}
				len--;
				cursor--;

				write(STDOUT_FILENO, "\x1b[D", 3);
				write(STDOUT_FILENO, "\x1b[s", 3);
				write(STDOUT_FILENO, buf + cursor, len - cursor);
				write(STDOUT_FILENO, " ", 1);
				write(STDOUT_FILENO, "\x1b[u", 3);
				break;
			case KEY_ENTER:
				write(STDOUT_FILENO, "\n", 1);
				finished = 1;
				break;
			case KEY_CTRL_D:
				if(len == 0) {
					free(buf);
					*str = NULL;
					return 0;
				}

				if(cursor < len) {
					for(size_t i = cursor; i < len - 1; ++i){
						buf[i] = buf[i + 1];
					}
					len--;
					write(STDOUT_FILENO, "\x1b[s", 3);
					write(STDOUT_FILENO, buf + cursor, len - cursor);
					write(STDOUT_FILENO, " ", 1);
					write(STDOUT_FILENO, "\x1b[u", 3);
				}
				break;
			case KEY_CTRL_C:
				write(STDOUT_FILENO, "^C\n", 3);
				len = 0;
				cursor = 0;
				finished = 1;
				break;
			case KEY_CTRL_Z:
			case KEY_CTRL_L:
				write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
				reprint_greeting();
				break;
			case KEY_UP:
				const char *next = history_next();
				if(next){
					if(cursor > 0){
						char esc[16];
						int n = snprintf(esc, sizeof(esc), "\x1b[%zuD", cursor);
						write(STDOUT_FILENO, esc, n);
					}	
			
					write(STDOUT_FILENO, "\x1b[K", 3);
					strcpy(buf, next);
					len = strlen(next);
					cursor = len;
					write(STDOUT_FILENO, buf, len);
				}	
				break;
			case KEY_DOWN:
				const char *prev = history_prev();
				if(prev){
					if(cursor > 0){
						char esc[16];
						int n = snprintf(esc, sizeof(esc), "\x1b[%zuD", cursor);
						write(STDOUT_FILENO, esc, n);
					}

					write(STDOUT_FILENO, "\x1b[K", 3);
					strcpy(buf, prev);
					len = strlen(prev);
					cursor = len;
					write(STDOUT_FILENO, buf, len);
				}
				break;
			case KEY_LEFT:
				if (cursor > 0) {
					write(STDOUT_FILENO, "\x1b[D", 3);
					cursor--;
				}
				break;
			case KEY_RIGHT:
				if (cursor < len) {
					write(STDOUT_FILENO, "\x1b[C", 3);
					cursor++;
				}
				break;
			case KEY_ESC:
			default:
				break;
		}
	}
	buf[len] = '\0';
	return len;
}

