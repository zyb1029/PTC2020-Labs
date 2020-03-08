%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-02-21 */
  #define FLEXDEBUG false // <- debugging switch for flex
%}

%option yylineno

%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdbool.h>
  #include "syntax.tab.h"
  #if FLEXDEBUG
  void printType(const char*);
  #define TOKENIFY(t) printType("t")
  #else
  #define printType(t) /* t */
  #define TOKENIFY(t) return yylval.type = t
  #endif
  #define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
  int yycolumn = 1;
  void printErrorA(const char*, bool);
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
      printErrorA("unterminated comment", false);
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
{type} { TOKENIFY(TYPE); }
{int} { yylval.ival = atoi(yytext); TOKENIFY(INT); }
{float} { yylval.fval = atof(yytext); TOKENIFY(FLOAT); }
{invalidnum} { printErrorA("invalid number", true); }
{id} { TOKENIFY(ID); }
; { TOKENIFY(SEMI); }
, { TOKENIFY(COMMA); }
= { TOKENIFY(ASSIGNOP); }
{relop} {
  switch (yytext[0]) {
    case '=':
      yylval.rval = RELOP_EQ;
      break;
    case '!':
      yylval.rval = RELOP_NE;
      break;
    case '<':
      yylval.rval = yyleng == 1 ? RELOP_LT : RELOP_LE;
      break;
    case '>':
    default:
      yylval.rval = yyleng == 1 ? RELOP_GT : RELOP_GE;
  }
  TOKENIFY(RELOP);
}
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
. { printErrorA("unknown character", true); }

%%

#if FLEXDEBUG
void printType(const char* type) { printf("%s ", type); }
#endif
void printErrorA(const char* message, bool showText) {
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
