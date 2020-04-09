#include <stdio.h>
#include <assert.h>
#include "tree.h"
#include "syntax.tab.h"

void printSyntaxTree() {
  printSyntaxTreeAux(stroot, 0);
}

void printSyntaxTreeAux(STNode *node, int indent) {
  if (node->empty) return;
  for (int i = 0; i < indent; ++i) printf("  ");

  assert(node->name != NULL);
  printf("%s", node->name);
  if (node->token == -1) {
    /* print lineno for symbols */
    printf(" (%d)", node->line);
  } else /* (node->token != -1) */ {
    /* print value/name for tokens */
    switch (node->token) {
      case INT:
        printf(": %u", node->ival);
        break;
      case FLOAT:
        printf(": %f", node->fval);
        break;
      case ID:
      case TYPE:
        printf(": %s", node->sval);
        break;
      default:
        break;
    }
  }
  printf("\n");

  for (STNode *child = node->child; child != NULL; child = child->next) {
    printSyntaxTreeAux(child, indent + 1);
  }
}

void teardownSyntaxTree(STNode *node) {
  for (STNode *child = node->child; child != NULL; child = child->next) {
    teardownSyntaxTree(child);
  }
  free(node);
}
