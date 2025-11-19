#pragma once

#include <token.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

////////////////////////////////////

const char metachars[] = "<>&|;(){}";

const char *operators[] = {
	"<<<", "<<", "<&", "<>", "<", ">>", ">&", ">",
	"&&", "&", "||", "|&", "|", ";",
	"((", "))", "(", ")", "{", "}", "\0",
         NULL	
};

////////////////////////////////////

Token *tokenize(const char*);
Token extract(const char*, size_t*);
Token extract_str(const char*, size_t*);
Token extract_op(const char*, size_t*);
Token extract_word(const char*, size_t*);
int word_end(const char *, size_t);

void print_tokens(Token*);
void clear_tokens(Token*);

/////////////////////////////////////

int word_end(const char *str, size_t index){
	return str[index] != '\0' && !isspace(str[index]) && !strchr(metachars, str[index]); 
}

Token extract_word(const char *str, size_t *index){
	size_t start = *index;
	while(word_end(str, *index)) ++*index;
	char *word = strndup(&str[start], *index - start);
	if(!word){
		perror("extract_word: ");
		Token token = {TOKEN_ERROR, str + *index};
		return token;
	}
	Token token = {TOKEN_WORD, word};
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

Token extract_str(const char *str, size_t *index){
	size_t start = *index;
	++*index;
	while(str[*index] != '\0' && str[*index] != '\"') ++*index;
	if(str[*index] == '\0'){
		fprintf(stderr, "Not even closed \"\n");
		Token token = {TOKEN_ERROR, str + *index};
		return token;
	}
	++*index;
	char *word = strndup(&str[start], *index - start);
	if(!word){
		perror("extract_str: ");
		Token token = {TOKEN_ERROR, str + *index};
		return token;
	}
	Token token = {TOKEN_WORD, word};
	return token;
}

Token extract(const char *str, size_t *index){
	while(isspace(str[*index])) ++*index;
	Token comment = {TOKEN_EOF, NULL};
	if(str[*index] == '#') return comment;
	else if(str[*index] == '\"') return extract_str(str, index);
	else if(strchr(metachars, str[*index])) return extract_op(str, index);
	return extract_word(str, index);
}

// Creating Array of Tokens and extracting every Token from string
Token *tokenize(const char *str){
	size_t size = 0, index = 0, count = 0;
	while(str[size] != '\n' && str[size] != '\0' && str[size] != '#') ++size;
	Token *result = malloc((size+1) * sizeof(Token));
	if(!result){
		perror("tokenize");
		return NULL;
	}
	while(index < size){
		result[count] = extract(str, &index);
		if(result[count].type == TOKEN_ERROR) break;
		++count;
	}
	Token end = {TOKEN_EOF, NULL};
	result[count] = end;

	result = realloc(result, sizeof(Token)*(count+1));
	if(!result){
		perror("tokenize");
		return NULL;
	}
	return result;
}



// Easy output of every Token
void print_tokens(Token *tokens){
	for(size_t i = 0; tokens[i].type != TOKEN_EOF; ++i){
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
