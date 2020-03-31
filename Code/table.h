/**
 * The symbol table/stack structure.
 * Copyright, Tianyun Zhang, 2020/03/30.
 * */

#ifndef TABLE_H
#define TABLE_H

#include "rbtree.h"
#include "type.h"

// Be careful, STNode is already taken in 'tree.c'.
typedef struct STEntry {
  const char *id;
  SEType *type;
} STEntry;

typedef struct STStack {
  struct RBNode *root;
  struct STStack *prev; // no next
} STStack;

void STPrepare();
void STDestroy();

void STPushStack();
void STPopStack();

void STInsertBase(const char *id, SEType *type);
void STInsertCurr(const char *id, SEType *type);

STEntry *STSearch(const char *id);
STEntry *STSearchBase(const char *id);
STEntry *STSearchCurr(const char *id);
STEntry *STSearchAt(STStack *stack, STEntry *target);

// helpers
int STRBCompare(const void *p1, const void *p2);
void STRBDestroy(void *p);

#endif // TABLE_H
