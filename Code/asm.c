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

const char *_header =
  "    .data\n"
  "_prompt: .asciiz \"Enter an integer:\"\n"
  "_ret: .asciiz \"\\n\"\n"
  "\n"
  "    .text\n"
  "    .globl main\n"
  "\n"
  "read:\n"
  "    li      $v0,4\n"
  "    la      $a0,_prompt\n"
  "    syscall\n"
  "    li      $v0,5\n"
  "    syscall\n"
  "    jr      $ra\n"
  "\n"
  "write:\n"
  "    li      $v0,1\n"
  "    syscall\n"
  "    li      $v0,4\n"
  "    la      $a0,_ret\n"
  "    syscall\n"
  "    move    $v0,$0\n"
  "    jr      $ra\n\n";

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
  fprintf(file, "%s", _header);
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
  switch (code->kind) {
  case IR_CODE_LABEL: 
    fprintf(file, "label%d:\n", code->label.label.number);
    break;
  case IR_CODE_FUNCTION: {
    size_t size = code->function.function.size;
    fprintf(file, "%s:\n", code->function.function.name);
    fprintf(file, "    subu    $sp,$sp,%lu\n", size);
    fprintf(file, "    sw      $ra,%lu($sp)\n", size - 4);
    fprintf(file, "    sw      $fp,%lu($sp)\n", size - 8);
    fprintf(file, "    addiu   $fp,$sp,%lu\n", size);
    break;
  }
  case IR_CODE_ASSIGN:
    ASLoadRegister(file, _t0, code->assign.right);
    ASSaveRegister(file, _t0, code->assign.left);
    break;
  case IR_CODE_ADD:
    ASLoadRegister(file, _t0, code->binop.op1);
    ASLoadRegister(file, _t1, code->binop.op2);
    fprintf(file, "    add     %s,%s,%s\n", _t0, _t0, _t1);
    ASSaveRegister(file, _t0, code->binop.result);
    break;
  case IR_CODE_SUB:
    ASLoadRegister(file, _t0, code->binop.op1);
    ASLoadRegister(file, _t1, code->binop.op2);
    fprintf(file, "    sub     %s,%s,%s\n", _t0, _t0, _t1);
    ASSaveRegister(file, _t0, code->binop.result);
    break;
  case IR_CODE_MUL:
    ASLoadRegister(file, _t0, code->binop.op1);
    ASLoadRegister(file, _t1, code->binop.op2);
    fprintf(file, "    mul     %s,%s,%s\n", _t0, _t0, _t1);
    ASSaveRegister(file, _t0, code->binop.result);
    break;
  case IR_CODE_DIV:
    ASLoadRegister(file, _t0, code->binop.op1);
    ASLoadRegister(file, _t1, code->binop.op2);
    fprintf(file, "    div     %s,%s\n", _t0, _t1);
    fprintf(file, "    mflo    %s\n", _t0);
    ASSaveRegister(file, _t0, code->binop.result);
    break;
  case IR_CODE_LOAD:
    ASLoadRegister(file, _t1, code->load.right);
    fprintf(file, "    lw      %s,0(%s)\n", _t0, _t1);
    ASSaveRegister(file, _t0, code->load.left);
    break;
  case IR_CODE_SAVE:
    ASLoadRegister(file, _t0, code->save.right);
    ASLoadRegister(file, _t1, code->save.left);
    fprintf(file, "    sw      %s,0(%s)", _t0, _t1);
    break;
  case IR_CODE_JUMP:
    fprintf(file, "    j       label%d\n", code->jump.dest.number);
    break;
  case IR_CODE_JUMP_COND: {
    char command[8] = "";
    switch (code->jump_cond.relop.relop) {
    case RELOP_IV:
      Panic("invalid relop RELOP_IV");
      break;
    case RELOP_LT:
      sprintf(command, "blt");
      break;
    case RELOP_LE:
      sprintf(command, "ble");
      break;
    case RELOP_GE:
      sprintf(command, "bge");
      break;
    case RELOP_GT:
      sprintf(command, "bgt");
      break;
    case RELOP_EQ:
      sprintf(command, "beq");
      break;
    case RELOP_NE:
      sprintf(command, "bne");
      break;
    }
    ASLoadRegister(file, _t0, code->jump_cond.op1);
    ASLoadRegister(file, _t1, code->jump_cond.op2);
    fprintf(file, "    %s     %s,%s,label%d\n", command, _t0, _t1, code->jump_cond.dest.number);
    break;
  }
  case IR_CODE_RETURN: {
    Assert(code->parent != NULL, "code not belong to function");
    size_t size = code->parent->function.function.size;
    ASLoadRegister(file, _v0, code->ret.value);
    fprintf(file, "    lw      $fp,%lu($sp)\n", size - 8);
    fprintf(file, "    lw      $ra,%lu($sp)\n", size - 4);
    fprintf(file, "    addiu   $sp,$sp,%lu\n", size);
    fprintf(file, "    jr      $ra\n");
    break;
  }
  /*
  IR_CODE_DEC,
  IR_CODE_ARG,
  */
  case IR_CODE_CALL:
    fprintf(file, "    jal     %s\n", code->call.function.name);
    // ASMoveRegister(file, _) // DO NOT MOVE, just save it
    ASSaveRegister(file, _v0, code->call.result);
    break;
  /*
  IR_CODE_PARAM,
  */
  case IR_CODE_READ:
    fprintf(file, "    jal     read\n");
    ASSaveRegister(file, _v0, code->read.variable);
    break;
  case IR_CODE_WRITE:
    ASLoadRegister(file, _a0, code->write.variable);
    fprintf(file, "    jal     write\n");
    break;
  default:
    break;
  }
}

// Move value between registers.
void ASMoveRegister(FILE *file, const char *to, const char *from) {
  fprintf(file, "    move    $%s,$%s", to, from);
}

// Load value to register.
void ASLoadRegister(FILE *file, const char *reg, IROperand var) {
  if (var.kind == IR_OP_CONSTANT) {
    fprintf(file, "    li      $%s,%d\n", reg, var.ivalue);
  } else {
    fprintf(file, "    lw      $%s,-%lu($fp)\n", reg, var.offset);
  }
}

// Save value to memory.
void ASSaveRegister(FILE *file, const char *reg, IROperand var) {
  fprintf(file, "    sw      $%s,-%lu($fp)\n", reg, var.offset);
}

// Prepare function's variables and stack size.
size_t ASPrepareFunction(IRCode *func, RBNode **root) {
  if (func == NULL) { 
    return 0;
  }

  size_t size = 8; // 4 for $ra, 4 for $fp
  for (IRCode *code = func->next; code != NULL && code->kind != IR_CODE_FUNCTION; code = code->next) {
    code->parent = func;
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
