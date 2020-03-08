#include <stdio.h>
#include "tree.h"

extern void yyrestart(FILE *);
extern int yyparse_wrap(); // defined in syntax.y

STNode* stroot;

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    fprintf(stderr, "Usage: parser file\n");
    return 1;
  }
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 2;
  }
  yyrestart(f);
  yyparse_wrap();
  printSyntaxTree();
  return 0;
}
