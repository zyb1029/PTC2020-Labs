/**
 * The semantic type/struct structure.
 * Copyright, Tianyun Zhang 2020/03/30.
 * */

#ifndef SE_TYPE_H
#define SE_TYPE_H

#include <stdbool.h>

enum SEBasicType {
  BASIC,
  ARRAY,
  STRUCTURE,
  FUNCTION,
};

// Forward declarations
typedef struct STNode STNode;
typedef struct SEField SEField;

typedef struct SEType {
  enum SEBasicType kind;
  union {
    int basic;
    struct {
      int size;
      struct SEType *elem;
    } array;
    struct SEField *structure;
  };
} SEType;

typedef struct SEField {
  const char *name;
  struct SEType *type;
  struct SEField *next;
} SEField;

SEType *SEParseExp(STNode *exp);
SEType *SEParseSpecifier(STNode *specifier);
SEField *SEParseDefList(STNode *list, bool assignable);
SEField *SEParseDef(STNode *def, SEField *tail, bool assignable);
SEField *SEParseDecList(STNode *list, SEType *type, SEField *tail, bool assignable);
SEField *SEParseDec(STNode *dec, SEType *type, bool assignable);
SEField *SEParseVarDec(STNode *var, SEType *type);

bool SECompareType(const SEType *t1, const SEType *t2);
SEType *SECopyType(const SEType *type);
void SEDestroyType(SEType *type);

#endif // SE_TYPE_H
