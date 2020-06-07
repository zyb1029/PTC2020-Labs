#include "asm.h"
#include "ir.h"
#include "rbtree.h"

#define DEBUG // <- assembler debug switch
#include "debug.h"

extern IRCodeList irlist;

// External API to translate IR to MIPS.
void assemble(FILE *file) {
  ASTranslateList(file, irlist);
}

// Internal API to translate IR to MIPS.
void ASTranslateList(FILE *file, IRCodeList list) {
  for (IRCode *code = list.head; code != NULL; code = code->next) {
    if (code->kind == IR_CODE_FUNCTION) {
      code->function.function.size = ASPrepareFunction(code, &code->function.root);
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
  size_t size = 0;
  switch (code->kind) {
  case IR_CODE_LABEL: 
    fprintf(file, "label%d:\n", code->label.label.number);
    break;
  case IR_CODE_FUNCTION:
    size = code->function.function.size;
    fprintf(file, "%s:\n", code->function.function.name);
    fprintf(file, "  subu $sp,$sp,%lu\n", size + 8);
    break;
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

// Prepare function's variables and stack size.
size_t ASPrepareFunction(IRCode *func, RBNode **root) {
  if (func == NULL) { 
    return 0;
  }

  size_t size = 0;
  for (IRCode *code = func->next; code != NULL; code = code->next) {
    switch (code->kind) {
      case IR_CODE_ASSIGN:
        size += ASRegisterVariable(&code->assign.left, root, size);
        size += ASRegisterVariable(&code->assign.right, root, size);
        break;
      case IR_CODE_ADD:
      case IR_CODE_SUB:
      case IR_CODE_MUL:
      case IR_CODE_DIV:
        size += ASRegisterVariable(&code->binop.result, root, size);
        size += ASRegisterVariable(&code->binop.op1, root, size);
        size += ASRegisterVariable(&code->binop.op2, root, size);
        break;
      case IR_CODE_LOAD:
        size += ASRegisterVariable(&code->load.left, root, size);
        size += ASRegisterVariable(&code->load.right, root, size);
        break;
      case IR_CODE_SAVE:
        size += ASRegisterVariable(&code->save.left, root, size);
        size += ASRegisterVariable(&code->save.right, root, size);
        break;
      case IR_CODE_JUMP_COND:
        size += ASRegisterVariable(&code->jump_cond.op1, root, size);
        size += ASRegisterVariable(&code->jump_cond.op2, root, size);
        break;
      case IR_CODE_RETURN:
        size += ASRegisterVariable(&code->ret.value, root, size);
        break;
      case IR_CODE_DEC:
        size += ASRegisterVariable(&code->dec.variable, root, size);
        break;
      case IR_CODE_READ:
        size += ASRegisterVariable(&code->read.variable, root, size);
        break;
      case IR_CODE_WRITE:
        size += ASRegisterVariable(&code->write.variable, root, size);
        break;
    default:
      break;
    }
  }
  return size;
}

// Register a new local variable and allocate it on memory.
size_t ASRegisterVariable(IROperand *op, RBNode **root, size_t offset) {
  switch (op->kind) {
    case IR_OP_TEMP:
    case IR_OP_VARIABLE: {
      if (!RBContains(root, op, ASComp)) {
        op->offset = offset;
        RBInsert(root, op, ASComp);
        Log("new variable %s%d, size %lu", op->kind == IR_OP_TEMP ? "t" : "v",
                                           op->number, op->size);
        return op->size;
      }
      break;
    }
    case IR_OP_MEMBLOCK: {
      op->offset = offset;
      return op->size;
    }
    default:
      break;
  }
  return 0;
}

int ASComp(const void *a, const void *b) {
  const IROperand *op1 = (const IROperand *)a;
  const IROperand *op2 = (const IROperand *)b;
  if (op1->kind == op2->kind && op1->number == op2->number) {
    return 0;
  } else if ((op1->kind == IR_OP_TEMP && op2->kind != IR_OP_TEMP) ||
             (op1->kind == op2->kind && op1->number < op2->number)) {
    return -1;
  } else {
    return 1;
  }
}
