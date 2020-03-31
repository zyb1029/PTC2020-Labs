/**
 * The semantic errors and functions.
 * Copyright, Tianyun Zhang, 2020/03/30.
 * */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "tree.h"
#include "rbtree.h"
#include "type.h"

enum SemanticErrors {
  SE_PSEUDO_ERROR                   = 0,
  SE_VARIABLE_UNDEFINED             = 1,
  SE_FUNCTION_UNDEFINED             = 2,
  SE_VARIABLE_DUPLICATE             = 3,
  SE_FUNCTION_DUPLICATE             = 4,
  SE_MISMATCHED_ASSIGNMENT          = 5,
  SE_RVALUE_ASSIGNMENT              = 6,
  SE_MISMATCHED_OPERANDS            = 7,
  SE_MISMATCHED_RETURN              = 8,
  SE_MISMATCHED_SIGNATURE           = 9,
  SE_ACCESS_TO_NON_ARRAY            = 10,
  SE_ACCESS_TO_NON_FUNCTION         = 11,
  SE_NON_INTEGER_INDEX              = 12,
  SE_ACCESS_TO_NON_STRUCT           = 13,
  SE_STRUCT_FIELD_UNDEFINED         = 14,
  SE_STRUCT_FIELD_DUPLICATE         = 15,
  SE_STRUCT_FIELD_INITIALIZED       = 15, // same ID
  SE_STRUCT_DUPLICATE               = 16,
  SE_STRUCT_UNDEFINED               = 17,
  SE_FUNCTION_DECLARED_NOT_DEFINED  = 18,
  SE_FUNCTION_CONFLICTING_SIGNATURE = 19,
};

typedef struct STError {
  enum SemanticErrors id;
  bool showID;
  const char *message1;
  const char *message2;
} STError;

void semanticScan();
void checkSemantics(STNode *node, STNode *parent);
void throwErrorS(enum SemanticErrors id, STNode *node);

#endif // SEMANTIC_H
