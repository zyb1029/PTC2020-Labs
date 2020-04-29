/**
 * The intermediate representation.
 * Copyright, Tianyun Zhang, 2020/04/29.
 * */

#ifndef IR_H
#define IR_H

enum IROperandType {
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
    int var_no;
    int value;
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

void IRAppendCode(struct IRCodeList *list, struct IRCode *code);
void IRConcatLists(struct IRCodeList *list1, struct IRCodeList *list2);
void IRDestroyList(struct IRCodeList *list);

#endif
