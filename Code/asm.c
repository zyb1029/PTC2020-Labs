#include "asm.h"
#include "ir.h"
#include "rbtree.h"

extern IRCodeList irlist;

// External API to translate IR to MIPS.
void assemble(FILE *file) {
  ASTranslateList(file, irlist);
}

// Internal API to translate IR to MIPS.
void ASTranslateList(FILE *file, IRCodeList list) {
  for (IRCode *code = list.head; code != NULL; code = code->next) {
    if (code->kind == IR_CODE_FUNCTION) {
      code->function.root = (RBNode *)malloc(sizeof(RBNode));
      ASPrepareFunction(code, code->function.root);
    }
  }
  for (IRCode *code = list.head; code != NULL; code = code->next) {
    ASTranslateCode(file, code);
  }
  for (IRCode *code = list.head; code != NULL; code = code->next) {
    if (code->kind == IR_CODE_FUNCTION) {
      RBDestroy(&code->function.root, NULL);
    }
  }
}

// Translate a single code to MIPS assembly.
void ASTranslateCode(FILE *file, IRCode *code) {
  switch (code->kind) {
  case IR_CODE_LABEL: 
    fprintf(file, "label%d:\n", code->label.label.number);
    break;
  /*
  case IR_CODE_FUNCTION:
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

// Prepare function's variables and stack size.
size_t ASPrepareFunction(IRCode *func, RBNode *root) {
  if (func == NULL) { 
    return 0;
  }

  size_t size = 0;
  for (IRCode *code = func->next; code != NULL; code = code->next) {
    switch (code->kind) {
      /*
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
  return size;
}

size_t ASRegisterVariable(IROperand op, RBNode *root, size_t offset) {
  switch (op.kind) {
    case IR_OP_TEMP: {

    }
    case IR_OP_VARIABLE: {

    }
    case IR_OP_MEMBLOCK: {

    }
    default:
      break;
  }
  return 0;
}
