/**
 * The intermediate representation.
 * Copyright, Tianyun Zhang, 2020/04/29.
 * */

#ifndef IR_H
#define IR_H

#define IRDebug true // <- debug switch
#if IRDebug
#define DEBUG
#endif

struct STNode;

enum IROperandType {
  IR_OP_NULL,
  IR_OP_TEMP,
  IR_OP_VARIABLE,
  IR_OP_CONSTANT,
  IR_OP_ADDRESS,
  IR_OP_LABEL,
};

enum IRCodeType {
  IR_CODE_ASSIGN,
  IR_CODE_ADD,
  IR_CODE_SUB,
  IR_CODE_MUL,
  IR_CODE_DIV,
  IR_CODE_LABEL,
  IR_CODE_JUMP,
  IR_CODE_JUMP_COND,
  IR_CODE_RETURN,
};

typedef struct IROperand {
  enum IROperandType kind;
  union {
    int number;
    int ivalue;
    float fvalue;
    const char *name;
    unsigned int address;
  };
} IROperand;

typedef struct IRCode {
  enum IRCodeType kind;
  union {
    struct {
      struct IROperand label;
    } label;
    struct {
      struct IROperand right, left;
    } assign;
    struct {
      struct IROperand result, op1, op2;
    } binop;
    struct {
      struct IROperand dest;
    } jump;
    struct {
      struct IROperand op1, relop, op2, dest;
    } jump_cond;
    struct {
      struct IROperand value;
    } ret;
  };
  struct IRCode *prev, *next;
} IRCode;

typedef struct IRCodeList {
  struct IRCode *head, *tail;
} IRCodeList;

struct IRCodeList IRTranslateExp(struct STNode *exp, struct IROperand place);
struct IRCodeList IRTranslateCondPre(struct STNode *exp, struct IROperand place);
struct IRCodeList IRTranslateCond(struct STNode *exp, struct IROperand label_true, struct IROperand label_false);
struct IRCodeList IRTranslateCompSt(struct STNode *compst);
struct IRCodeList IRTranslateStmt(struct STNode *stmt);

struct IROperand IRNewNullOperand();
struct IROperand IRNewTempOperand();
struct IROperand IRNewLabelOperand();
struct IROperand IRNewVariableOperand(struct STNode *id);
struct IROperand IRNewConstantOperand(int value);
struct IRCode *IRNewCode(enum IRCodeType kind);
struct IRCodeList IRWrapCode(struct IRCode *code);
struct IRCodeList IRAppendCode(struct IRCodeList list, struct IRCode *code);
struct IRCodeList IRConcatLists(struct IRCodeList list1, struct IRCodeList list2);
void IRDestroyList(struct IRCodeList list);

#endif
