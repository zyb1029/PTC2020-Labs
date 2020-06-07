#include "asm.h"
#include "ir.h"

extern IRCodeList irlist;

void assemble(FILE *file) {
  ASTranslateList(file, irlist);
}

void ASTranslateList(FILE *file, IRCodeList list) {
  for (IRCode *code = list.head, *next = NULL; code != NULL; code = next) {
    next = code->next;
    ASTranslateCode(file, code);
  }
}

void ASTranslateCode(FILE *file, IRCode *code) {
  switch (code->kind) {
  case IR_CODE_LABEL: 
    fprintf(file, "label%d:\n", code->label.label.number);
    break;
    /*
  }
  IR_CODE_FUNCTION,
  IR_CODE_ASSIGN,
  IR_CODE_ADD,
  IR_CODE_SUB,
  IR_CODE_MUL,
  IR_CODE_DIV,
  IR_CODE_LOAD,
  IR_CODE_SAVE,
  IR_CODE_JUMP,
  IR_CODE_JUMP_COND,
  IR_CODE_RETURN,
  IR_CODE_DEC,
  IR_CODE_ARG,
  IR_CODE_CALL,
  IR_CODE_PARAM,
  IR_CODE_READ,
  IR_CODE_WRITE,
  */
  default:
    break;
  }
}
