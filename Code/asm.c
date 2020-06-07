#include "asm.h"
#include "ir.h"
#include "rbtree.h"

#define DEBUG // <- assembler debug switch
#include "debug.h"

extern IRCodeList irlist;

const char *registers[] = {
  "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
  "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

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
#ifdef DEBUG
  fprintf(file, "# ");
  IRWriteCode(file, code);
#endif
  size_t size = 0;
  switch (code->kind) {
  case IR_CODE_LABEL: 
    fprintf(file, "label%d:\n", code->label.label.number);
    break;
  case IR_CODE_FUNCTION:
    size = code->function.function.size;
    fprintf(file, "%s:\n", code->function.function.name);
    fprintf(file, "    subu  $sp,$sp,%lu\n", size);
    fprintf(file, "    sw    $ra,%lu($sp)\n", size - 4);
    fprintf(file, "    sw    $fp,%lu($sp)\n", size - 8);
    fprintf(file, "    addiu $fp,$sp,%lu\n", size);
    break;
  case IR_CODE_ASSIGN:
    ASLoadRegister(file, _t0, code->assign.right);
    ASSaveRegister(file, _t0, code->assign.left);
    break;
  /*
  IR_CODE_ADD,
  IR_CODE_SUB,
  IR_CODE_MUL,
  IR_CODE_DIV,
  IR_CODE_LOAD,
  IR_CODE_SAVE,
  */
  case IR_CODE_JUMP:
    fprintf(file, "    j     label%d\n", code->jump.dest.number);
    break;
  /*
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

// Load value to register.
void ASLoadRegister(FILE *file, const char *reg, IROperand var) {
  if (var.kind == IR_OP_CONSTANT) {
    fprintf(file, "    li    $%s,%d\n", reg, var.ivalue);
  } else {
    fprintf(file, "    lw    $%s,-%lu($fp)\n", reg, var.offset);
  }
}

// Save value to memory.
void ASSaveRegister(FILE *file, const char *reg, IROperand var) {
  fprintf(file, "    sw    $%s,-%lu($fp)\n", reg, var.offset);
}

// Prepare function's variables and stack size.
size_t ASPrepareFunction(IRCode *func, RBNode **root) {
  if (func == NULL) { 
    return 0;
  }

  size_t size = 8; // 4 for $ra, 4 for $fp
  for (IRCode *code = func->next; code != NULL && code->kind != IR_CODE_FUNCTION; code = code->next) {
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
        op->offset = offset + op->size;
        RBInsert(root, op, ASComp);
        Log("new variable %s%d, size %lu, offset %lu", op->kind == IR_OP_TEMP ? "t" : "v",
                                                       op->number, op->size, op->offset);
        return op->size;
      } else {
        RBNode *node = RBSearch(root, op, ASComp);
        Assert(node != NULL && node->value != NULL, "bad data in RB tree");
        op->offset = ((const IROperand *)node->value)->offset;
      }
      break;
    }
    case IR_OP_MEMBLOCK: {
      op->offset = offset + op->size;
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
