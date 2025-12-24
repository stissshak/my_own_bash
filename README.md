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
    - echo, cd, pwd, bexit, help 
    - jobs, fg, bg, bkill
    - history
- History
- Job control
- Colors

## Every file expained here
- token.h - structure of token and tokens type
- lexer.h and lexer.c are creating tokens (Word, Operators, Redirects, Utility tools)
- ast.h and ast.c are store funcs to create AST and work with it (Nodes: command, binary(and, or, pipes, background), unary(group, subshell))
- parser.h and parser.c are parsing array of tokens and return AST
- func.h and func.c are contain some of built-in funcs and structure Function (echo, cd, pwd, bexit, help)
- job.h and job.c are realising job_structure and funcs for it (structure of job_t, funcs for it: fg, bg, bkill)
- exec.c and exec.h are executing AST from parser
- terminal.h and terminal.c are set terminal to raw mode and realise all keys
- history.h and history.c have created for history of terminal
- sash.c - main file that contains all of files and preparing bash to work

## Plans to do
- Autofill
- Colors for user writed text
- Optimization
