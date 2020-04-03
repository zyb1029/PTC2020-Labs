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

enum STStackKind {
  STACK_GLOBAL,    // global stack, destroy all contents.
  STACK_LOCAL,     // local stack, do NOT destroy structures.
  STACK_STRUCTURE, // structure stack, do NOT destroy types.
};
typedef struct STStack {
  enum STStackKind kind;
  struct RBNode *root;
  struct STStack *prev; // no next
} STStack;

void STPrepare();
void STDestroy();

void STPushStack(enum STStackKind kind);
void STPopStack();

void STInsertStru(const char *id, SEType *type);
void STInsertFunc(const char *id, SEType *type);
void STInsertGlob(const char *id, SEType *type);
void STInsertCurr(const char *id, SEType *type);

STEntry *STSearch(const char *id);
STEntry *STSearchStru(const char *id);
STEntry *STSearchFunc(const char *id);
STEntry *STSearchGlob(const char *id);
STEntry *STSearchCurr(const char *id);
STEntry *STSearchAt(STStack *stack, STEntry *target);

// helpers
int STRBCompare(const void *p1, const void *p2);
void STRBDestroy(void *p);

#endif // TABLE_H
