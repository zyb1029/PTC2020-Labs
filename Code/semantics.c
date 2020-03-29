#include "semantics.h"
#include "syntax.tab.h"
#define SEMANTIC_DEBUG true // <- debug switch

extern bool hasErrorS;
extern STNode *stroot;

STStack *baseStack = NULL;
STStack *currStack = NULL;

// main entry of semantic scan
void semanticScan() {
  // prepare the base stack
  baseStack = (STStack *)malloc(sizeof(STStack));
  baseStack->root = baseStack->prev = NULL;
  currStack = baseStack;
  checkSemantics(stroot, stroot);
}

void checkSemantics(STNode *node, STNode *parent) {
  if (node->empty) return;
  if (node->token == ID) {
    printf("%s %s:%s\n", parent->name, node->name, node->sval);
  }
  for (STNode *child = node->child; child != NULL; child = child->next) {
    checkSemantics(child, node);
  }
}

