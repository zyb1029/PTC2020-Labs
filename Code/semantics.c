#include "semantics.h"
#include "syntax.tab.h"
#define SEMANTIC_DEBUG true // <- debug switch

extern bool hasErrorS;
extern STNode *stroot;

// main entry of semantic scan
void semanticScan() {
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

