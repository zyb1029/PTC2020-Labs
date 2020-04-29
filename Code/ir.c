#include "ir.h"
#include "tree.h"
#include "table.h"
#include "token.h"
#include "syntax.tab.h"
#include "debug.h"

// same assertion code as in type.c
#ifdef DEBUG
#define AssertSTNode(node, str) \
  Assert(node, "node is null"); \
  Assert(!strcmp(node->name, str), "not a " str);
#else
#define AssertSTNode(node, str)
#endif

static unsigned int IRTempNumber = 0;
const IRCodeList STATIC_EMPTY_IR_LIST = { NULL, NULL };

// Translate an Exp into IRCodeList.
IRCodeList IRTranslateExp(STNode *exp, IROperand place) {
  AssertSTNode(exp, "Exp");
  STNode *e1 = exp->child;
  STNode *e2 = e1 ? e1->next : NULL;
  STNode *e3 = e2 ? e2->next : NULL;
  switch (e1->token) {
    case LP: // LP Exp RP
      Panic("not implemented!");
    case MINUS: {
      Panic("not implemented!");
    }
    case NOT: {
      Panic("not implemented!");
    }
    case ID: {
      if (e2 == NULL) {
        IRCode *code = IRNewCode(ASSIGN);
        code->assign.left = place;
        code->assign.right = IRNewVariableOperand(e1);
        return IRWrapCode(code);
      } else {
        // function call
        Panic("not implemented!");
      }
      break;
    }
    case INT:
    case FLOAT: {
      IRCode *code = IRNewCode(ASSIGN);
      IRCodeList list = { code, code };
      code->assign.left = place;
      code->assign.right = IRNewConstantOperand(e1);
      return IRWrapCode(code);
    }
    default: {
      switch (e2->token) {
        case LB: {
          Panic("not implemented!");
        }
        case DOT: {
          Panic("not implemented!");
        }
        case ASSIGNOP: {
          IROperand t1 = IRNewTempOperand();
          IROperand var = IRNewVariableOperand(e1);
          IRCodeList list = IRTranslateExp(e3, t1);
          
          IRCode *code1 = IRNewCode(ASSIGN);
          code1->assign.left = var;
          code1->assign.right = t1;

          IRCode *code2 = IRNewCode(ASSIGN);
          code2->assign.left = place;
          code2->assign.right = var;
          
          list = IRAppendCode(list, code1);
          list = IRAppendCode(list, code2);
          return list;
        }
        case AND:
        case OR: {
          Panic("not implemented!");
        }
        case RELOP: {
          Panic("not implemented!");
        }
        default: {
          Panic("not implemented!");
        }
      }
    }
  }
  Panic("should not reach here");
  return STATIC_EMPTY_IR_LIST;
}

// Allocate a new temporary operand.
IROperand IRNewTempOperand() {
  IROperand op;
  op.kind = TEMP;
  op.number = IRTempNumber++;
  return op;
}

// Generate a new variable operand.
IROperand IRNewVariableOperand(STNode *id) {
  AssertSTNode(id, "ID");
  IROperand op;
  op.kind = VARIABLE;
  op.name = id->sval;
  return op;
}

// Generate a new constant operand.
IROperand IRNewConstantOperand(STNode *constant) {
  IROperand op;
  op.kind = CONSTANT;
  switch (constant->token) {
    case INT:
      op.ivalue = constant->ival;
      break;
    case FLOAT:
      op.fvalue = constant->fval;
      break;
    default:
      Panic("invalid constant token");
  }
  return op;
}

// Allocate memory and initialize a code.
IRCode *IRNewCode(enum IRCodeType kind) {
  IRCode *code = (IRCode *)malloc(sizeof(IRCode));
  code->kind = kind;
  code->prev = code->next = NULL;
  return code;
}

// Wrap a single code to IRCodeList.
IRCodeList IRWrapCode(IRCode *code) {
  IRCodeList list;
  list.head = code;
  list.tail = code;
  return list;
}

// Append a code to the end of list.
IRCodeList IRAppendCode(IRCodeList list, IRCode *code) {
  IRCodeList ret = { list.head, list.tail };  
  if (ret.tail == NULL) {
    ret.head = ret.tail = code;
  } else {
    ret.tail->next = code;
    code->prev = ret.tail;
    ret.tail = code;
  }
  return ret;
}

// Concat list2 to the end of list1.
IRCodeList IRConcatLists(IRCodeList list1, IRCodeList list2) {
  if (list1.tail == NULL) {
    return list2;  // might be empty
  } else {
    list1.tail->next = list2.head;
    list2.head->prev = list1.tail;
    list1.tail = list2.tail;
    return list1;
  }
}

// Destory all codes in an IRCodeList.
void IRDestroyList(IRCodeList list) {
  // Do not free the IRCodeList, it is static
  for (IRCode *code = list.head, *next = NULL; code != NULL; code = next) {
    next = code->next;  // safe loop
    free(code);
  }
}
