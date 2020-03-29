#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "sytree.h"
#include "rbtree.h"

// Be careful, STNode is already taken in 'tree.c'.
typedef struct STEntry {
  int type;
  const char* name;
  union {
    struct {
      int type;
    } variable;
    struct {
      int returnType;
      int NRParameters;
    } function;
  };
} STEntry;

typedef struct STStack {
  struct STEntry *root;
  struct STStack *prev; // no next
} STStack;

void semanticScan();
void checkSemantics(STNode *node, STNode *parent);

#endif // SEMANTIC_H
