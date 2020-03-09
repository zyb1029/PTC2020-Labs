%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-02-21 */
  #define FLEXDEBUG false // <- debugging switch for flex
%}

%option yylineno

%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdbool.h>
  #include "token.h"
  #include "tree.h"
  #include "syntax.tab.h"
  #if FLEXDEBUG
  void printType(const char*);
  #define TOKENIFY(t) printType("t")
  #else
  #define printType(t) do { /* t */ } while (0)
  #define TOKENIFY(t)                                   \
    do {                                                \
      STNode *node = (STNode *)malloc(sizeof(STNode));  \
      node->line   = yylineno;                          \
      node->column = yycolumn;                          \
      node->token  = t;                                 \
      node->symbol = -1;     /* to be translated */     \
      node->name   = NULL;                              \
      node->empty  = false;                             \
      switch (t) {                                      \
        case INT:                                       \
          node->ival = yylval.ival;                     \
          break;                                        \
        case FLOAT:                                     \
          node->fval = yylval.fval;                     \
          break;                                        \
        case RELOP:                                     \
          node->rval = yylval.rval;                     \
          break;                                        \
        case ID:                                        \
        case TYPE:                                      \
          strcpy(node->sval, yylval.sval);              \
          break;                                        \
        default:                                        \
          break; /* undefined value */                  \
      }                                                 \
      yylloc.st_node = node;                            \
      return yylval.type = t;                           \
    } while (0)
  #endif
  #define YY_USER_ACTION                                \
    yylloc.first_line   = yylloc.last_line = yylineno;  \
    yylloc.first_column = yycolumn;                     \
    yylloc.last_column  = yycolumn + yyleng - 1;        \
    yylloc.st_node      = NULL;                         \
    yycolumn += yyleng;
  int yycolumn = 1;
  void throwErrorA(const char*, bool);
%}

decdigit   [0-9]
decdigit0  [1-9]
octdigit   [0-7]
octdigit0  [1-7]
hexdigit   {decdigit}|[a-fA-F]
hexdigit0  {decdigit0}|[a-fA-F]
letter     [a-zA-Z]
letter_    {letter}|_
character  [a-zA-Z0-9]
character_ {character}|_
relop      >|<|>=|<=|==|!=
type       int|float
newline    \n
whitespace [ \r\n\t]
int        0|{decdigit0}{decdigit}*|0{octdigit}+|0[xX]{hexdigit}+
floatN     {decdigit}+\.{decdigit}+
floatE     ({decdigit}+\.{decdigit}*|\.{decdigit}+)[eE][+-]?{decdigit}+
float      {floatN}|{floatE}
invalidnum \.?{decdigit}({character}|\.)*
id         {letter_}{character_}*
commentl   \/\/.*
commentb   \/\*

%%

{commentl} { printType("COMMENT-LINE"); }
{commentb} {
  printType("COMMENT-BLOCK");
  int ch;
  while (true) {
    ch = input();
    ++yycolumn;
    if (!ch || ch == EOF) {
      throwErrorA("unterminated comment", false);
      yyterminate();
    } else if (ch == '*') {
      do {
        ch = input();
        ++yycolumn;
      } while (ch == '*');
      if (ch == '/') break;
    }
    if (ch == '\n') yycolumn = 1;
  }
}
{newline} { 
#if FLEXDEBUG
  printf("\n");
#endif
  yycolumn = 1;
}
{whitespace} { /* do nothing */ }
struct { TOKENIFY(STRUCT); }
return { TOKENIFY(RETURN); }
if { TOKENIFY(IF); }
else { TOKENIFY(ELSE); }
while { TOKENIFY(WHILE); }
{type} { ACTYYLVAL(s); TOKENIFY(TYPE); }
{int} { SETYYLVAL(i); TOKENIFY(INT); }
{float} { SETYYLVAL(f); TOKENIFY(FLOAT); }
{invalidnum} { throwErrorA("invalid number", true); }
{id} { ACTYYLVAL(s); TOKENIFY(ID); }
; { TOKENIFY(SEMI); }
, { TOKENIFY(COMMA); }
= { TOKENIFY(ASSIGNOP); }
{relop} { SETYYLVAL(r); TOKENIFY(RELOP); }
\+ { TOKENIFY(PLUS); }
- { TOKENIFY(MINUS); }
\* { TOKENIFY(STAR); }
\/ { TOKENIFY(DIV); }
&& { TOKENIFY(AND); }
\|\| { TOKENIFY(OR); }
\. { TOKENIFY(DOT); }
! { TOKENIFY(NOT); }
\( { TOKENIFY(LP); }
\) { TOKENIFY(RP); }
\[ { TOKENIFY(LB); }
\] { TOKENIFY(RB); }
\{ { TOKENIFY(LC); }
\} { TOKENIFY(RC); }
<<EOF>> {
  TOKENIFY(EOF);
#if FLEXDEBUG
  printf("\n");
#endif
  yyterminate();
}
. { throwErrorA("unknown character", true); }

%%

#if FLEXDEBUG
void printType(const char* type) { printf("%s ", type); }
#endif
extern int  errLineno;
extern bool hasErrorA;
void throwErrorA(const char* message, bool showText) {
  hasErrorA = true;
  if (errLineno == yylineno) return; // one error per line
  else errLineno = yylineno;
  printf("Error type A at Line %d: %s", yylineno, message);
  if (showText) {
    printf(" \'%s\'", yytext);
  }
  printf("\n");
}
#if FLEXDEBUG
int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: lexical file");
    return -1;
  } else {
    if (!(yyin = fopen(argv[1], "r"))) {
      perror(argv[1]);
      return -2;
    } else {
      yylex();
      return 0;
    }
  }
}
#endif
