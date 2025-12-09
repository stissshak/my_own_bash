# my_own_bash

## Here some main points written
- Lexer and creating tokens
- Parser and parsing of tokens
- Execution of AST:
    - Command and redirects
    - Group
    - Logical
    - Pipe 
- Buil-in functions:
    - pwd
    - jobs, fg, bg
    - history
- History
- Job control

## Every file expained here
- token.h - structure of token and tokens type
- lexer.h and lexer.c are creating tokens
- ast.h and ast.c are store funcs to create AST and work with it
- parser.h and parser.c are parsing array of tokens and return AST
- func.h and func.c are contain some of built-in funcs and structure Function
- job.h and job.c are realising job_structure and funcs for it
- exec.c and exec.h are executing AST from parser
- terminal.h and terminal.c are set terminal to raw mode and realise all keys
- history.h and history.c have created for history of terminal
- sash.c - main file that contains all of files and preparing bash to work

## Plans to do
1. Termianl settings (Colors)
