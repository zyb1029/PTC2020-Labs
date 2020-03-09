#include <stdio.h>
#include <stdbool.h>
#include "tree.h"

extern void yyrestart(FILE *);
extern int yyparse_wrap(); // defined in syntax.y

bool hasErrorA = false;
bool hasErrorB = false;
STNode* stroot = NULL;

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

  if (!hasErrorA && !hasErrorB) printSyntaxTree();

  return 0;
}
