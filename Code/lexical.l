%{
    /* Copyright, Tianyun Zhang @ Nanjing University. 2020-02-21 */
%}

%{
    #include <stdio.h>
    int chars = 0, words = 0, lines = 0;
%}

digit  [0-9]
letter [a-zA-Z]

%%

{letter}+ { chars += yyleng; ++words; }
\n        { ++chars; ++lines; }
.         { ++chars; }

%%

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
            printf("%d %d %d\n", chars, words, lines);
            return 0;
        }
    }
}
