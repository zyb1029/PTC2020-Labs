%{
    /* Copyright, Tianyun Zhang @ Nanjing University. 2020-02-21 */
%}

%option yylineno

%{
    #include <stdio.h>
    void printType(const char*);
%}

digit      [0-9]
letter     [a-zA-Z]
character  [a-zA-Z0-9]
character_ {character}|_
relop      >|<|>=|<=|==|!=
type       int|float
whitespace [ \r\n\t]

%%

{whitespace} { /* do nothing */ }
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
{type} { printType("TYPE"); }
\( { printType("LP"); }
\) { printType("RP"); }
\[ { printType("LB"); }
\] { printType("RB"); }
\{ { printType("LC"); }
\} { printType("RC"); }
struct { printType("STRUCT"); }
return { printType("RETURN"); }
if { printType("IF"); }
else { printType("ELSE"); }
while { printType("WHILE"); }

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
