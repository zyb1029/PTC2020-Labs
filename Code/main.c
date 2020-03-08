#include <stdio.h>
#include "tree.h"

extern void yyrestart(FILE *);
extern int _warp_yyparse(); // defined in syntax.y

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
  _warp_yyparse();
  printSyntaxTree();
  return 0;
}
