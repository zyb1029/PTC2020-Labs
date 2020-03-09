#include <stdio.h>
#include "tree.h"
#include "syntax.tab.h"

void printSyntaxTree() {
  printSyntaxTreeAux(stroot, 0);
}

void printSyntaxTreeAux(STNode *node, int indent) {
  for (int i = 0; i < indent; ++i) printf("  ");
  if (node->name) printf("%s", node->name);
  switch (node->token) {
    case INT:
      printf(": %d\n", node->ival);
      break;
    case FLOAT:
      printf(": %f\n", node->fval);
      break;
    case ID:
    case TYPE:
      printf(": %s\n", node->sval);
      break;
    default:
      printf("\n");
      break;
  }
  for (STNode *child = node->child; child != NULL; child = child->next) {
    printSyntaxTreeAux(child, indent + 1);
  }
}

