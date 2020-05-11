#include "opt.h"
#include "ir.h"
#include "rbtree.h"

#define DEBUG
#include "debug.h"

extern IRCodeList irlist;

int timestamp = 0;
int valid_ts = -1;
RBNode *OCRoot = NULL;

// Optimize the constants.
void optimize() {
  int line = 0;
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
      OCUpdateOperand(&code->assign.right);
      if (code->assign.right.kind == IR_OP_CONSTANT) {
        OCInsert(code->assign.left, code->assign.right.ivalue);
      }
      break;
    }
    case IR_CODE_JUMP_COND: {
      OCUpdateOperand(&code->jump_cond.op1);
      OCUpdateOperand(&code->jump_cond.op2);
      break;
    }
    case IR_CODE_WRITE: {
      OCUpdateOperand(&code->write.variable);
      break;
    }
    case IR_CODE_RETURN: {
      OCUpdateOperand(&code->ret.value);
      break;
    }
    default:
      break;
    }
  }
}

// Find the constant from the RB tree.
OCNode *OCFind(IROperand op) {
  OCNode node;
  node.is_var = op.kind == IR_OP_VARIABLE;
  node.number = op.number;
  RBNode *target = RBSearch(&OCRoot, &node, OCComp);
  if (target == NULL) {
    return NULL;
  } else {
    return OCComp(target->value, &node) == 0 ? target->value : NULL;
  }
}

// Optimize an operand with constant value if possible.
// Return true if the operand is replaced by a constant.
bool OCUpdateOperand(IROperand *op) {
  if (op->kind == IR_OP_TEMP || op->kind == IR_OP_VARIABLE) {
    OCNode *node = OCFind(*op);
    if (node != NULL && node->timestamp >= valid_ts) {
      *op = IRNewConstantOperand(node->value);
      return true;
    }
  }
  return false;
}

// Insert a new or update a constant into RB tree.
void OCInsert(IROperand op, int value) {
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
    RBInsert(&OCRoot, node, OCComp);
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
