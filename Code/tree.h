/**
 * The syntax tree structure.
 * Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07
 * */

#ifndef TREE_H
#define TREE_H

#define STDEBUG false // <- syntax tree debugger switch

#include <stdbool.h>
#include "token.h"

typedef struct STNode {
  int line, column;
  int token, symbol;
  const char* name;
  bool empty;
  union {
    int             ival;
    float           fval;
    enum ENUM_RELOP rval;
    char            sval[64];
  };
  struct STNode *child, *next;
} STNode;

extern STNode *stroot;

void printSyntaxTree();
void printSyntaxTreeAux(STNode *, int);

#endif

