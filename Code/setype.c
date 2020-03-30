#include <string.h>
#include "sytree.h"
#include "setype.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

// Create type from specifier node
SEType *SECreateType(STNode *node) {
  Assert(node, "node is null");
  Assert(!strcmp(node->name, "Specifier"), "node is not specifier");
  STNode *child = node->child;
  SEType *type = (SEType *)malloc(sizeof(SEType));
  switch (child->token) {
    case TYPE:
      type->kind = BASIC;
      if (!strcmp(child->sval, "int")) {
        Log("INT");
        type->basic = INT;
      } else {
        Log("FLOAT");
        type->basic = FLOAT;
      }
      break;
    case STRUCT:
      Log("STRUCT");
      Panic("Please implement me!");
    default:
      Panic("Illegal token type");
  }
  return type; 
}

bool SECompareType(const SEType *t1, const SEType *t2) {
  // TODO
  Panic("Please implement me");
  return false;
}

void SEDestroyType(SEType *type) {
  // TODO
  Panic("Please implement me");
  return;
}
