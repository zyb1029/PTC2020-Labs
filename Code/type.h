/**
 * The semantic type/struct structure.
 * Copyright, Tianyun Zhang 2020/03/30.
 * */

#ifndef SE_TYPE_H
#define SE_TYPE_H

#include <stdbool.h>

enum SEBasicType {
  VOID,
  BASIC,
  ARRAY,
  STRUCTURE,
  FUNCTION,
};

// Forward declarations
typedef struct STNode STNode;
typedef struct SEField SEField;

typedef struct SEType {
  bool extended; // should not be destroyed in local
  enum SEBasicType kind;
  union {
    int basic;
    struct {
      int size;
      enum SEBasicType kind; // type may already be destroyed!
      struct SEType *type;
    } array;
    struct SEField *structure;
    struct {
      STNode *node;
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

SEType *SEParseExp(STNode *exp);
SEType *SEParseSpecifier(STNode *specifier);

void SEParseExtDefList(STNode *list);
void SEParseExtDef(STNode *edef);
void SEParseExtDecList(STNode *list, SEType *type);
void SEParseFunDec(STNode *fdec, SEType *type);

void SEParseCompSt(STNode *comp, SEType *type);
void SEParseStmtList(STNode *list, SEType *type);
void SEParseStmt(STNode *stmt, SEType *type);

// not assignable == function signature, or struct definition
SEFieldChain SEParseDefList(STNode *list, bool assignable);
SEFieldChain SEParseDef(STNode *def, bool assignable);
SEFieldChain SEParseDecList(STNode *list, SEType *type, bool assignable);
SEFieldChain SEParseDec(STNode *dec, SEType *type, bool assignable);
SEFieldChain SEParseVarDec(STNode *var, SEType *type, bool assignable);
SEFieldChain SEParseVarList(STNode *list);
SEFieldChain SEParseParamDec(STNode *pdec);
SEFieldChain SEParseArgs(STNode *args);

void SEDumpType(const SEType *type);
bool SECompareType(const SEType *t1, const SEType *t2);
bool SECompareField(const SEField *f1, const SEField *f2);
void SEDestroyType(SEType *type);
void SEDestroyField(SEField *field);

#endif // SE_TYPE_H
