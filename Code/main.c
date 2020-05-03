#include <stdio.h>
#include <stdbool.h>
#include "tree.h"
#include "semantics.h"
#include "ir.h"

extern void yyrestart(FILE *);
extern int yyparse_wrap(); // defined in syntax.y

int  errLineno = 0;
bool hasErrorA = false;
bool hasErrorB = false;
bool hasErrorS = false;
STNode* stroot = NULL;
IRCodeList irlist = {NULL, NULL};

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

  // Step 1: call yyparse to get syntax tree.
  yyrestart(f);
  yyparse_wrap();
  if (hasErrorA || hasErrorB) {
    return 3;
  }
  //printSyntaxTree();

  // Step 2: conduct a full semantic scan.
  // Step 3: translate to IR during the scan.
  semanticScan();
  teardownSyntaxTree(stroot);
  if (hasErrorS) {
    return 4; // avoid checker see this as runtime error
  }

  return 0;
}
