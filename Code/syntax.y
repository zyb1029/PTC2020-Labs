%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07 */
%}

%{
  #include <stdio.h>
  #include "lex.yy.c"
  void yyerror(char *);
%}

%token INT FLOAT
%token ADD SUB MUL DIV

%%
Term : INT | FLOAT;

%%
void yyerror(char *msg) {
  fprintf(stderr, "yyerror: %s\n", msg);
}