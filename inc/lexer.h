// lexer.h

#pragma once

#include "token.h"
#include <stddef.h>

////////////////////////////////////

Token *tokenize(const char*);
Token extract(const char*, size_t*, size_t);
Token extract_str(const char*, size_t*);
Token extract_op(const char*, size_t*);
Token extract_word(const char*, size_t*);
int word_end(const char *, size_t);

void print_tokens(Token*);
void clean_tokens(Token*);

/////////////////////////////////////


