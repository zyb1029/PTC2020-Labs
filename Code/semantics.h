/**
 * The semantic symbol table/stack structure.
 * Copyright, Tianyun Zhang, 2020/03/30.
 * */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "sytree.h"
#include "rbtree.h"
#define SEMANTIC_DEBUG true // <- debug switch

typedef struct STError {
  int id;
  bool showId;
  const char *message1;
  const char *message2;
} STError;

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
void printErrorS(int id, STNode *node);

#endif // SEMANTIC_H
