#include "opt.h"
#include "ir.h"
#include "rbtree.h"

// #define DEBUG // <- optimizer debugging switch
#include "debug.h"

extern IRCodeList irlist;

int timestamp = 0;
int valid_ts = -1;
RBNode *OCRoot = NULL;

// Optimize the constants.
void optimize() {
  // Step 1: replace all values with constants if possible
  Log("optimization step 1");
  for (IRCode *code = irlist.head, *next = NULL; code != NULL; code = next) {
    next = code->next;
    switch (code->kind) {
    case IR_CODE_LABEL:
    case IR_CODE_FUNCTION: {
      // invalid all constants
      valid_ts = ++timestamp;
      break;
    }
    case IR_CODE_ASSIGN: {
      Log("assign");
      OCCreate(code->assign.left);
      OCReplace(&code->assign.right);
      OCInvalid(code->assign.left);
      if (code->assign.right.kind == IR_OP_CONSTANT) {
        Log("operand updated, type %d, number %d", code->assign.left.kind,
            code->assign.left.number);
        OCUpdate(code->assign.left, code->assign.right.ivalue);
      }
      break;
    }
    case IR_CODE_ADD:
    case IR_CODE_SUB:
    case IR_CODE_MUL:
    case IR_CODE_DIV: {
      OCCreate(code->binop.result);
      OCReplace(&code->binop.op1);
      OCReplace(&code->binop.op2);
      OCInvalid(code->binop.result);
      // special case: do not handle dividing by zero
      if (code->binop.op1.kind == IR_OP_CONSTANT &&
          code->binop.op2.kind == IR_OP_CONSTANT &&
          (code->kind != IR_CODE_DIV || code->binop.op2.ivalue != 0)) {
        int val = 0;
        IROperand result = code->binop.result;
        switch (code->kind) {
        case IR_CODE_ADD:
          val = code->binop.op1.ivalue + code->binop.op2.ivalue;
          break;
        case IR_CODE_SUB:
          val = code->binop.op1.ivalue - code->binop.op2.ivalue;
          break;
        case IR_CODE_MUL:
          val = code->binop.op1.ivalue * code->binop.op2.ivalue;
          break;
        case IR_CODE_DIV:
          val = code->binop.op1.ivalue / code->binop.op2.ivalue;
          break;
        default:
          Panic("should not reach here");
        }
        code->kind = IR_CODE_ASSIGN;
        code->assign.left = result;
        code->assign.right = IRNewConstantOperand(val);
        OCUpdate(result, val);
      }
      break;
    }
    case IR_CODE_LOAD: {
      // do not optimize address
      OCCreate(code->load.left);
      OCInvalid(code->load.left);
      break;
    }
    case IR_CODE_SAVE: {
      OCCreate(code->save.left);
      OCReplace(&code->save.right);
      OCInvalid(code->save.left);
      break;
    }
    case IR_CODE_JUMP:
      break;
    case IR_CODE_JUMP_COND: {
      OCReplace(&code->jump_cond.op1);
      OCReplace(&code->jump_cond.op2);
      break;
    }
    case IR_CODE_RETURN: {
      OCReplace(&code->ret.value);
      break;
    }
    case IR_CODE_DEC: {
      OCCreate(code->dec.variable);
      OCInvalid(code->dec.variable);
      break;
    }
    case IR_CODE_ARG: {
      OCReplace(&code->arg.variable);
      break;
    }
    case IR_CODE_CALL: {
      OCCreate(code->call.result);
      OCInvalid(code->call.result);
      break;
    }
    case IR_CODE_PARAM: {
      OCCreate(code->param.variable);
      OCInvalid(code->param.variable);
      break;
    }
    case IR_CODE_READ: {
      OCCreate(code->read.variable);
      OCInvalid(code->read.variable);
      break;
    }
    case IR_CODE_WRITE: {
      OCReplace(&code->write.variable);
      break;
    }
    default:
      Panic("should not reach here");
    }
  }

  // Step 2: replace all values with variables if possible
  Log("optimization step 2");
  valid_ts = ++timestamp;
  for (IRCode *code = irlist.head, *next = NULL; code != NULL; code = next) {
    next = code->next;
    switch (code->kind) {
    case IR_CODE_LABEL:
    case IR_CODE_FUNCTION: {
      // invalid all variables
      valid_ts = ++timestamp;
      break;
    }
    case IR_CODE_ASSIGN: {
      OCReplace2(&code->assign.right);
      OCInvalid(code->assign.left);
      if (code->assign.right.kind == IR_OP_TEMP) {
        Log("operand updated, TEM, number %d", code->assign.left.number);
        OCUpdate2(code->assign.left, code->assign.right.number, TEM);
      } else if (code->assign.right.kind == IR_OP_VARIABLE) {
        Log("operand updated, VAR, number %d", code->assign.left.number);
        OCUpdate2(code->assign.left, code->assign.right.number, VAR);
      } else if (code->assign.right.kind == IR_OP_VADDRESS) {
        Log("operand updated, ADD, number %d", code->assign.left.number);
        OCUpdate2(code->assign.left, code->assign.right.number, ADD);
      } else if (code->assign.right.kind == IR_OP_MEMBLOCK) {
        Log("operand updated, MEM, number %d", code->assign.left.number);
        OCUpdate2(code->assign.left, code->assign.right.number, MEM);
      }
      break;
    }
    case IR_CODE_ADD:
    case IR_CODE_SUB:
    case IR_CODE_MUL:
    case IR_CODE_DIV: {
      OCReplace2(&code->binop.op1);
      OCReplace2(&code->binop.op2);
      OCInvalid(code->binop.result);
      break;
    }
    case IR_CODE_LOAD: {
      // do not optimize address
      OCInvalid(code->load.left);
      break;
    }
    case IR_CODE_SAVE: {
      OCReplace2(&code->save.right);
      OCInvalid(code->save.left);
      break;
    }
    case IR_CODE_JUMP:
      break;
    case IR_CODE_JUMP_COND: {
      OCReplace2(&code->jump_cond.op1);
      OCReplace2(&code->jump_cond.op2);
      break;
    }
    case IR_CODE_RETURN: {
      OCReplace2(&code->ret.value);
      break;
    }
    case IR_CODE_DEC:
      break;
    case IR_CODE_ARG: {
      OCReplace2(&code->arg.variable);
      break;
    }
    case IR_CODE_CALL:
    case IR_CODE_PARAM:
    case IR_CODE_READ:
      break;
    case IR_CODE_WRITE: {
      OCReplace2(&code->write.variable);
      break;
    }
    default:
      Panic("should not reach here");
    }
  }

  // Step 3: mark all important variables
  Log("optimization step 3");
  for (IRCode *code = irlist.tail, *prev = NULL; code != NULL; code = prev) {
    prev = code->prev;
    switch (code->kind) {
    case IR_CODE_LABEL:
    case IR_CODE_FUNCTION:
      break;
    case IR_CODE_ASSIGN: {
      OCNode *node = OCFind(code->assign.left);
      if (node->important) {
        OCImportant(code->assign.right);
      }
      break;
    }
    case IR_CODE_ADD:
    case IR_CODE_SUB:
    case IR_CODE_MUL:
    case IR_CODE_DIV: {
      OCNode *node = OCFind(code->binop.result);
      if (node->important) {
        OCImportant(code->binop.op1);
        OCImportant(code->binop.op2);
      }
      break;
    }
    case IR_CODE_LOAD: {
      OCImportant(code->load.right);
      break;
    }
    case IR_CODE_SAVE: {
      OCImportant(code->save.left);
      OCImportant(code->save.right);
      break;
    }
    case IR_CODE_JUMP:
      break;
    case IR_CODE_JUMP_COND: {
      OCImportant(code->jump_cond.op1);
      OCImportant(code->jump_cond.op2);
      break;
    }
    case IR_CODE_RETURN: {
      OCImportant(code->ret.value);
      break;
    }
    case IR_CODE_DEC: {
      OCImportant(code->dec.variable);
      break;
    }
    case IR_CODE_ARG: {
      OCImportant(code->arg.variable);
      break;
    }
    case IR_CODE_CALL:
      break;
    case IR_CODE_PARAM: {
      OCImportant(code->param.variable);
      break;
    }
    case IR_CODE_READ: {
      OCImportant(code->read.variable);
      break;
    }
    case IR_CODE_WRITE: {
      OCImportant(code->write.variable);
      break;
    }
    default:
      break;
      Panic("should not reach here");
    }
  }

  // Step 4: delete all inactive variables
  Log("optimization step 4");
  for (IRCode *code = irlist.tail, *prev = NULL; code != NULL; code = prev) {
    prev = code->prev;
    switch (code->kind) {
    case IR_CODE_LABEL:
    case IR_CODE_FUNCTION:
      break;
    case IR_CODE_ASSIGN: {
      OCNode *node = OCFind(code->assign.left);
      bool delete = node != NULL && !node->important && !node->active;
      OCDeactivate(code->assign.left);
      if (delete) {
        Log("remove ASSIGN");
        IRRemoveCode(irlist, code);
        free(code);
      } else {
        OCActivate(code->assign.right);
      }
      break;
    }
    case IR_CODE_ADD:
    case IR_CODE_SUB:
    case IR_CODE_MUL:
    case IR_CODE_DIV: {
      OCNode *node = OCFind(code->binop.result);
      bool delete = node != NULL && !node->important && !node->active;
      OCDeactivate(code->binop.result);
      if (delete) {
        Log("remove BINOP");
        IRRemoveCode(irlist, code);
        free(code);
      } else {
        OCActivate(code->binop.op1);
        OCActivate(code->binop.op2);
      }
      break;
    }
    case IR_CODE_LOAD: {
      OCNode *node = OCFind(code->load.left);
      bool delete = node != NULL && !node->important && !node->active;
      OCDeactivate(code->load.left);
      if (delete) {
        Log("remove LOAD");
        IRRemoveCode(irlist, code);
        free(code);
      } else {
        OCActivate(code->load.right);
      }
      break;
    }
    case IR_CODE_SAVE: {
      // cannot delete SAVE
      OCActivate(code->save.left);
      OCActivate(code->save.right);
      break;
    }
    case IR_CODE_JUMP:
      break;
    case IR_CODE_JUMP_COND: {
      OCActivate(code->jump_cond.op1);
      OCActivate(code->jump_cond.op2);
      break;
    }
    case IR_CODE_RETURN: {
      OCActivate(code->ret.value);
      break;
    }
    case IR_CODE_DEC: {
      OCDeactivate(code->dec.variable);
      break;
    }
    case IR_CODE_ARG: {
      OCActivate(code->arg.variable);
      break;
    }
    case IR_CODE_CALL: {
      OCDeactivate(code->call.result);
      break;
    }
    case IR_CODE_PARAM: {
      OCDeactivate(code->param.variable);
      break;
    }
    case IR_CODE_READ: {
      OCDeactivate(code->read.variable);
      break;
    }
    case IR_CODE_WRITE: {
      OCActivate(code->write.variable);
      break;
    }
    default:
      Panic("should not reach here");
    }
  }
}

// Optimize an operand with constant value if possible.
// Return true if the operand is replaced by a constant.
bool OCReplace(IROperand *op) {
  if (op->kind == IR_OP_CONSTANT) {
    return true;
  } else if (op->kind == IR_OP_TEMP || op->kind == IR_OP_VARIABLE ||
             op->kind == IR_OP_VADDRESS) {
    OCNode *node = OCFind(*op);
    if (node != NULL && node->timestamp >= valid_ts) {
      *op = IRNewConstantOperand(node->value);
      return true;
    }
  }
  return false;
}

// Optimize an operand with another variable if possible.
// Return true if the operand is replaced by a variable.
bool OCReplace2(IROperand *op) {
  if (op->kind == IR_OP_TEMP || op->kind == IR_OP_VARIABLE ||
      op->kind == IR_OP_VADDRESS) {
    OCNode *node = OCFind(*op);
    if (node != NULL && node->timestamp >= valid_ts) {
      switch (node->reserved) {
      case TEM:
        op->kind = IR_OP_TEMP;
        break;
      case VAR:
        op->kind = IR_OP_VARIABLE;
        break;
      case ADD:
        op->kind = IR_OP_VADDRESS;
        break;
      case MEM:
        op->kind = IR_OP_MEMBLOCK;
        break;
      default:
        Panic("should not reach here");
      }
      op->number = node->value > 0 ? node->value : -node->value;
      return true;
    }
  }
  return false;
}

// Create a new operand in RB and set invalid.
void OCCreate(IROperand op) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE ||
      op.kind == IR_OP_VADDRESS) {
    OCNode *target = OCFind(op);
    if (target == NULL) {
      OCNode *node = (OCNode *)malloc(sizeof(OCNode));
      node->is_var = op.kind != IR_OP_TEMP;
      node->number = op.number;
      node->value = 0;
      node->timestamp = -1;
      node->important = false;
      node->active = false;
      RBInsert(&OCRoot, node, OCComp);
    }
  }
}

// Update a value of node in RB tree.
void OCUpdate(IROperand op, int value) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE ||
      op.kind == IR_OP_VADDRESS) {
    OCNode *target = OCFind(op);
    if (target != NULL) {
      target->value = value;
      target->timestamp = timestamp;
    }
  }
}

// Update a value of node with reserved field in RB tree.
void OCUpdate2(IROperand op, int value, int reserved) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE) {
    OCNode *target = OCFind(op);
    if (target != NULL) {
      target->value = value;
      target->reserved = reserved;
      target->timestamp = timestamp;
    }
  }
}

// Find the constant from the RB tree.
OCNode *OCFind(IROperand op) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE ||
      op.kind == IR_OP_VADDRESS) {
    OCNode node;
    node.is_var = op.kind != IR_OP_TEMP;
    node.number = op.number;
    RBNode *target = RBSearch(&OCRoot, &node, OCComp);
    if (target != NULL) {
      return OCComp(target->value, &node) == 0 ? target->value : NULL;
    }
  }
  return NULL;
}

// Set an constant as invalid.
void OCInvalid(IROperand op) {
  OCNode *node = OCFind(op);
  if (node != NULL) {
    node->timestamp = -1;
  }
}

// Set an constant as important.
void OCImportant(IROperand op) {
  OCNode *node = OCFind(op);
  if (node != NULL) {
    node->important = true;
  }
}

// Set an operand as active.
void OCActivate(IROperand op) {
  OCNode *node = OCFind(op);
  if (node != NULL) {
    node->active = true;
  }
}

// Set an operand as inactive.
void OCDeactivate(IROperand op) {
  OCNode *node = OCFind(op);
  if (node != NULL) {
    node->active = false;
  }
}

// Compare two OC structures.
int OCComp(const void *a, const void *b) {
  const OCNode *oa = (const OCNode *)a;
  const OCNode *ob = (const OCNode *)b;
  if (oa->is_var == ob->is_var && oa->number == ob->number) {
    return 0;
  } else if ((!oa->is_var && ob->is_var) ||
             (oa->is_var == ob->is_var && oa->number < ob->number)) {
    return -1;
  } else {
    return 1;
  }
}
