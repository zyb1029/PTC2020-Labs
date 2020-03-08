#include <stdio.h>
#include "tree.h"

void printSyntaxTree() {
  printSyntaxTreeAux(stroot, 0);
}

void printSyntaxTreeAux(STNode *node, int indent) {
  for (int i = 0; i < indent; ++i) printf("  ");
  printf("%s\n", node->name);
  for (STNode *child = node->child; node != NULL; node = node->next) {
    printSyntaxTreeAux(child, indent + 1);
  }
}

