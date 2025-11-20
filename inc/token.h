// token.h

#pragma once

typedef enum{
	// Redirection
	TOKEN_LTLTLT,	// <<<
	TOKEN_LTLT,	// <<
	TOKEN_LEFT_AMP,	// <&
	TOKEN_LTGT,	// <>
	TOKEN_LT,	// <
	TOKEN_GTGT,	// >>
	TOKEN_RIGHT_AMP,// >&
	TOKEN_GT,	// >

	// Pipes and logic
	TOKEN_AND,	// &&
	TOKEN_BACK,	// &	
	TOKEN_OR,	// ||
	TOKEN_PIPE_AMP,	// |&
	TOKEN_BAR,	// |
	TOKEN_SEMICOLON,// ;
	
	// Grouping
	TOKEN_LLPAREN,	// ((
	TOKEN_RRPAREN,	// ))
	TOKEN_LPAREN,	// (
	TOKEN_RPAREN,	// )
	TOKEN_LBRACE,	// {
	TOKEN_RBRACE,	// }
	
	// Other
	TOKEN_ERROR,	// ToTracKErrors
	TOKEN_EOF, 	// EndOfFile
	
	TOKEN_WORD, 	// char*
} TokenType;

typedef struct{
	TokenType type;
	const char *op;
} Token;
