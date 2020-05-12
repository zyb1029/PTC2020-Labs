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
  int line = 0;

  // Step 1: replace all values with constants if possible
  Log("optimization step 1");
  for (IRCode *code = irlist.head, *next = NULL; code != NULL; code = next) {
    Log("line %d", ++line);
    next = code->next;
    switch (code->kind) {
    case IR_CODE_LABEL:
    case IR_CODE_FUNCTION: {
      // invalid all constants
      valid_ts = ++timestamp;
      break;
    }
    case IR_CODE_ASSIGN: {
      OCCreate(code->assign.left);
      OCInvalid(code->assign.left);
      if (OCReplace(&code->assign.right)) {
        OCInsert(code->assign.left, code->assign.right.ivalue);
      }
      break;
    }
    case IR_CODE_ADD:
    case IR_CODE_SUB:
    case IR_CODE_MUL:
    case IR_CODE_DIV: {
      OCCreate(code->binop.result);
      OCInvalid(code->binop.result);
      bool is_constant = true; // avoid short circuit
      is_constant &= OCReplace(&code->binop.op1);
      is_constant &= OCReplace(&code->binop.op2);
      if (is_constant && // do not handle dividing by zero
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
        OCInsert(result, val);
      }
      break;
    }
    case IR_CODE_REFER: {
      OCCreate(code->addr.left);
      OCInvalid(code->addr.left);
      break;
    }
    case IR_CODE_LOAD: {
      OCCreate(code->load.left);
      OCInvalid(code->load.left);
      break;
    }
    case IR_CODE_SAVE: {
      OCCreate(code->save.left);
      OCInvalid(code->save.left);
      OCReplace(&code->save.right);
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

  // Step 2: mark all important variables
  Log("optimization step 2");
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
    case IR_CODE_REFER:
      break;
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

  // Step 2: delete all inactive variables
  Log("optimization step 3");
  for (IRCode *code = irlist.tail, *prev = NULL; code != NULL; code = prev) {
    Log("line %d", line--);
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
    case IR_CODE_REFER: {
      OCNode *node = OCFind(code->addr.left);
      bool delete = node != NULL && !node->important && !node->active;
      OCDeactivate(code->addr.left);
      if (delete) {
        Log("remove REFER");
        IRRemoveCode(irlist, code);
        free(code);
      } else {
        OCActivate(code->addr.right);
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
  } else if (op->kind == IR_OP_TEMP || op->kind == IR_OP_VARIABLE) {
    OCNode *node = OCFind(*op);
    if (node != NULL && node->timestamp >= valid_ts) {
      *op = IRNewConstantOperand(node->value);
      return true;
    }
  }
  return false;
}

// Create a new operand in RB and set invalid.
void OCCreate(IROperand op) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE) {
    OCNode *target = OCFind(op);
    if (target == NULL) {
      OCNode *node = (OCNode *)malloc(sizeof(OCNode));
      node->is_var = op.kind == IR_OP_VARIABLE;
      node->number = op.number;
      node->value = 0;
      node->timestamp = -1;
      node->important = false;
      node->active = false;
      RBInsert(&OCRoot, node, OCComp);
    }
  }
}

// Insert a new or update a constant into RB tree.
void OCInsert(IROperand op, int value) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE) {
    OCNode *target = OCFind(op);
    if (target != NULL) {
      target->value = value;
      target->timestamp = timestamp;
    } else {
      OCNode *node = (OCNode *)malloc(sizeof(OCNode));
      node->is_var = op.kind == IR_OP_VARIABLE;
      node->number = op.number;
      node->value = value;
      node->timestamp = timestamp;
      node->important = false;
      node->active = false;
      RBInsert(&OCRoot, node, OCComp);
    }
  }
}

// Find the constant from the RB tree.
OCNode *OCFind(IROperand op) {
  if (op.kind == IR_OP_TEMP || op.kind == IR_OP_VARIABLE) {
    OCNode node;
    node.is_var = op.kind == IR_OP_VARIABLE;
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
