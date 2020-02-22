%{
    /* Copyright, Tianyun Zhang @ Nanjing University. 2020-02-21 */
%}

%option yylineno

%{
    #include <stdio.h>
    void printType(const char*);
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
int        -?({decdigit}+|0{octdigit}+|0x{hexdigit}+)
float      {decdigit}*\.{decdigit}+([eE][+-]?{decdigit}+)?
id         {letter_}{character_}*
commentl   \/\/[.^\n]*
commentb   \/\*([.^\*]|(\*[.^\/]))*\*\/

%%

{newline}    { printf("\n"); }
{whitespace} { /* do nothing */ }
{commentl} { printType("COMMENT-LINE"); }
{commentb} { printType("COMMENT-BLOCK"); }
struct { printType("STRUCT"); }
return { printType("RETURN"); }
if { printType("IF"); }
else { printType("ELSE"); }
while { printType("WHILE"); }
{type} { printType("TYPE"); }
{id} { printType("ID"); }
{int} { printType("INT"); }
{float} { printType("FLOAT"); }
; { printType("SEMI"); }
, { printType("COMMA"); }
= { printType("ASSIGNOP"); }
{relop} { printType("RELOP"); }
\+ { printType("PLUS"); }
- { printType("MINUS"); }
\* { printType("STAR"); }
\/ { printType("DIV"); }
&& { printType("AND"); }
\|\| { printType("OR"); }
\. { printType("DOT"); }
! { printType("NOT"); }
\( { printType("LP"); }
\) { printType("RP"); }
\[ { printType("LB"); }
\] { printType("RB"); }
\{ { printType("LC"); }
\} { printType("RC"); }

. {
    printf("Error type A at Line %d: unknown characters \'%s\'\n", yylineno, yytext);
  }

%%

void printType(const char* type) { printf("%s ", type); }
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: wc file");
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
