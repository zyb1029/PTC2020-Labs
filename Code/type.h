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
struct STNode;
struct SEField;

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

SEType *SECreateType(struct STNode *node);
bool SECompareType(const SEType *t1, const SEType *t2);
void SEDestroyType(struct SEType *type);

#endif // SE_TYPE_H
