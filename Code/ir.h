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
  TEMP,
  VARIABLE,
  CONSTANT,
  ADDRESS,
};

enum IRCodeType {
  ASSIGN,
  ADD,
  SUB,
  MUL,
  DIV,
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
      struct IROperand right, left;
    } assign;
    struct {
      struct IROperand result, op1, op2;
    } binop;
  };
  struct IRCode *prev, *next;
} IRCode;

typedef struct IRCodeList {
  struct IRCode *head, *tail;
} IRCodeList;

struct IRCodeList IRTranslateExp(struct STNode *exp, struct IROperand place);

struct IRCode *IRNewCode(enum IRCodeType kind);
struct IRCodeList IRWrapCode(struct IRCode *code);
struct IRCodeList IRAppendCode(struct IRCodeList list, struct IRCode *code);
struct IRCodeList IRConcatLists(struct IRCodeList list1, struct IRCodeList list2);
void IRDestroyList(struct IRCodeList list);

#endif
