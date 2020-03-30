#ifndef SE_TYPE_H
#define SE_TYPE_H

enum SEBasicType {
  BASIC,
  ARRAY,
  STRUCTURE,
  FUNCTION,
};

struct SEField; // forward declaration
typedef struct SEType {
  enum SEBasicType kind;
  union {
    int basic;
    struct {
      int size;
      struct Type *elem;
    } array;
    struct SEField *structure;
  };
} SEType;

typedef struct SEField {
  const char *name;
  struct SEType *type;
  struct SEField *next;
} SEField;

SEType *SECreateType(STNode *node);
bool SECompareType(const SEType *t1, const SEType *t2);
void SEDestroyType(SEType *type);

#endif // SE_TYPE_H
