// lexer.c

#include "lexer.h"
#include "token.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/////////////////////////////////////////////////

const char metachars[] = "<>&|;(){}";
const char *operators[] = {
    "<<<", "<<", "<&", "<>", "<", ">>", ">&", ">", "&>>", "&>",
    "&&", "&", "||", "|&", "|", ";",
    "(", ")", "{", "}",
    NULL    
};

/////////////////////////////////////////////////

static int is_metachar(char c){
    return c && strchr(metachars, c);
}

int word_end(const char *str, size_t index){
	return str[index] != '\0' && !isspace(str[index]) && !strchr(metachars, str[index]); 
}

static int push(char **buf, size_t *len, size_t *size, char c){
    if(*len + 1 >= *size){
        *size *= 2;
        char *new = realloc(*buf, *size);
        if(!new) return -1;
        *buf = new;
    }
    (*buf)[(*len)++] = c;
    return 0;
}

static int extract_quoted(const char *str, size_t *index, char **buf, size_t *len, size_t *size){
    char c = str[*index];
    ++*index;

    while(str[*index] && str[*index] != c){
        if(c == '"' && str[*index] == '\\' && str[*index + 1]){
            char next = str[*index + 1];
            if(next == '"' || next == '\\' || next == '`'){
                push(buf, len, size, next);
                *index += 2;
                continue;
            }
        }
        push(buf, len, size, str[*index]);
        ++*index;
    }

    if(!str[*index]){
        return -1;
    }
    ++*index;
    return 0;
}

Token extract_word(const char *str, size_t *index){
    size_t size = 64, len = 0;
	char *buf = malloc(size);
	if(!buf){
		Token token = {TOKEN_ERROR, NULL};
		return token;
	}

    while(str[*index] && !isspace(str[*index]) && !is_metachar(str[*index])){

        char c = str[*index];

        if(c == '"' || c == '\''){

            if(extract_quoted(str, index, &buf, &len, &size) < 0){
                free(buf);
                Token token = {TOKEN_ERROR, NULL};
                return token;
            }
            continue;
        }

        if(c == '\\' && str[*index + 1]){

            ++*index;
            push(&buf, &len, &size, str[*index]);
            ++*index;
            continue;
        }
 
        push(&buf, &len, &size, str[*index]);

        ++*index;
    }

    buf[len] = '\0';

    if(len == 0){
        free(buf);
        Token token = {TOKEN_EOF, NULL};
        return token;
    }
	Token token = {TOKEN_WORD, buf};
	return token;
}

Token extract_op(const char *str, size_t *index){
	for(size_t i = 0; operators[i] != NULL; ++i){
		size_t len = strlen(operators[i]);
		if(strncmp(operators[i], &str[*index], len) == 0){
			TokenType type = i;
			char *op = strndup(&str[*index], len);
			if(!op){
				perror("extract_op: ");
				Token token = {TOKEN_ERROR, str + *index};
				return token;
			}
			*index += len;
			Token token = {type, op};
			return token;
		}
	}
	++*index;
	fprintf(stderr, "Invalid operator at lexer - %c, %ld\n", str[*index - 1], *index);
	Token token = {TOKEN_ERROR, str + *index};
	return token;
}

Token extract(const char *str, size_t *index){
	while(isspace(str[*index]) && str[*index] != '\0') ++*index;
	
    Token end = {TOKEN_EOF, NULL};
	if(!str[*index] || str[*index] == '\n' || str[*index] == '#') return end;
	if(strchr(metachars, str[*index])) return extract_op(str, index);
    return extract_word(str, index);
}

// Creating Array of Tokens and extracting every Token from string
Token *tokenize(const char *str){
	size_t index = 0, count = 0, size = 16;
	
	Token *result = malloc(size * sizeof(Token));
	if(!result) return NULL;

	while(1){
		if(count >= size - 1){
			size *= 2;
			result = realloc(result, size * sizeof(Token));
			if(!result) return NULL;
		}
		result[count] = extract(str, &index);
		if(result[count].type == TOKEN_ERROR || result[count].type == TOKEN_EOF) break;
		++count;
	}
	return result;
}



// Easy output of every Token
void print_tokens(Token *tokens){
	for(size_t i = 0; tokens[i].type != TOKEN_EOF && tokens[i].type != TOKEN_ERROR; ++i){
		printf("%d - %s\n", (int)tokens[i].type, tokens[i].op);
	}
}

// Clean elements of Array and this Array
void clean_tokens(Token *tokens){
	for(size_t i = 0; tokens[i].type != TOKEN_EOF; ++i){
		free((void*)tokens[i].op);
	}
	free(tokens);
}

