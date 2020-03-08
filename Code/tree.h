/**
 * The syntax tree structure.
 * Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07
 * */

#ifndef TREE_H
#define TREE_H

#include "relop.h"

typedef struct Tree {
  int line, type;
  union {
    int             ival;
    float           fval;
    enum ENUM_RELOP eval;
  };
  struct Tree *child, *next;
} Tree;

#endif