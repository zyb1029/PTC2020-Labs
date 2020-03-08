/**
 * The syntax tree structure.
 * Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07
 * */

#ifndef TREE_H
#define TREE_H

#include "relop.h"

typedef struct STNode {
  int line, column, type;
  union {
    int             ival;
    float           fval;
    enum ENUM_RELOP rval;
  };
  struct STNode *child, *next;
} STNode;

extern STNode *stroot;

#endif
