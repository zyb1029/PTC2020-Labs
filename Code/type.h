/**
 * The semantic type/struct structure.
 * Copyright, Tianyun Zhang 2020/03/30.
 * */

#ifndef SE_TYPE_H
#define SE_TYPE_H

#include <stdbool.h>
#include <unistd.h>

enum SEBasicType {
  VOID,
  BASIC,
  ARRAY,
  STRUCTURE,
  FUNCTION,
};

// Forward declarations
struct STNode;
struct SEField;

typedef struct SEType {
  bool extended; // should not be destroyed in local
  size_t size; // size of memory occupied by the type
  enum SEBasicType kind;
  struct SEType *parent; // disjoint-set-unions, see massimo's m25
  union {
    int basic;
    struct {
      int size;
      enum SEBasicType kind; // type may already be destroyed!
      struct SEType *type;
    } array;
    struct SEField *structure;
    struct {
      struct STNode *node;
      bool defined;
      struct SEType *type;
      struct SEField *signature;
    } function;
  };
} SEType;

typedef struct SEField {
  const char *name;
  enum SEBasicType kind; // type may already be destroyed!
  struct SEType *type;
  struct SEField *next;
} SEField;

typedef struct SEFieldChain {
  struct SEField *head;
  struct SEField *tail;
} SEFieldChain;

void SEPrepare();

SEType *SEParseExp(struct STNode *exp);
SEType *SEParseSpecifier(struct STNode *specifier);

void SEParseExtDefList(struct STNode *list);
void SEParseExtDef(struct STNode *edef);
void SEParseExtDecList(struct STNode *list, SEType *type);
void SEParseFunDec(struct STNode *fdec, SEType *type);

void SEParseCompSt(struct STNode *comp, SEType *type);
void SEParseStmtList(struct STNode *list, SEType *type);
void SEParseStmt(struct STNode *stmt, SEType *type);

// not assignable == function signature, or struct definition
SEFieldChain SEParseDefList(struct STNode *list, bool assignable);
SEFieldChain SEParseDef(struct STNode *def, bool assignable);
SEFieldChain SEParseDecList(struct STNode *list, SEType *type, bool assignable);
SEFieldChain SEParseDec(struct STNode *dec, SEType *type, bool assignable);
SEFieldChain SEParseVarDec(struct STNode *var, SEType *type, bool assignable);
SEFieldChain SEParseVarList(struct STNode *list);
SEFieldChain SEParseParamDec(struct STNode *pdec);
SEFieldChain SEParseArgs(struct STNode *args);

void SEDumpType(const SEType *type);
SEType *SEGetParentType(SEType *t);
bool SECompareType(SEType *t1, SEType *t2);
bool SECompareField(SEField *f1, SEField *f2);
void SEDestroyType(SEType *type);
void SEDestroyField(SEField *field);

#endif // SE_TYPE_H
