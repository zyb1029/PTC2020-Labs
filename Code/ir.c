#include <stdbool.h>
#include <unistd.h>

#include "ir.h"
#include "tree.h"
#include "token.h"
#include "table.h"
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
static unsigned int IRLabelNumber = 0;
const IRCodeList STATIC_EMPTY_IR_LIST = {NULL, NULL};

// Translate an Exp into IRCodeList.
IRCodeList IRTranslateExp(STNode *exp, IROperand place) {
  AssertSTNode(exp, "Exp");
  STNode *e1 = exp->child;
  STNode *e2 = e1 ? e1->next : NULL;
  STNode *e3 = e2 ? e2->next : NULL;
  switch (e1->token) {
    case LP:  // LP Exp RP
      Panic("not implemented!");
    case MINUS: {
      IROperand t1 = IRNewTempOperand();
      IRCodeList list = IRTranslateExp(e2, t1);

      IRCode *code = IRNewCode(IR_CODE_SUB);
      code->binop.result = place;
      code->binop.op1 = IRNewConstantOperand(0);
      code->binop.op2 = t1;
      return IRAppendCode(list, code);
    }
    case NOT:
      return IRTranslateCondPre(exp, place);
    case ID: {
      if (e2 == NULL) {
        IRCode *code = IRNewCode(IR_CODE_ASSIGN);
        code->assign.left = place;
        code->assign.right = IRNewVariableOperand(e1);
        return IRWrapCode(code);
      } else {
        // function call
        if (e3->token == RP) {
          // ID()
          if (!strcmp(e1->sval, "read")) {
            IRCode *code = IRNewCode(IR_CODE_READ);
            code->read.variable = place;
            return IRWrapCode(code);
          } else {
            IRCode *code = IRNewCode(IR_CODE_CALL);
            code->call.result = place;
            code->call.function = IRNewFunctionOperand(e1->sval);
            return IRWrapCode(code);
          }
        } else {
          // ID(args...)
          IRCodeList arg_list = STATIC_EMPTY_IR_LIST;
          IRCodeList list = IRTranslateArgs(e3, &arg_list);

          if (!strcmp(e1->sval, "write")) {
            Assert(arg_list.head != NULL, "empty arguments to WRITE");
            IRCode *code = IRNewCode(IR_CODE_WRITE);
            code->write.variable = arg_list.head->arg.variable;
            list = IRAppendCode(list, code);
            IRDestroyList(arg_list);  // argument list no longer useful
            return list;
          } else {
            IRCode *code = IRNewCode(IR_CODE_CALL);
            code->call.result = place;
            code->call.function = IRNewFunctionOperand(e1->sval);
            list = IRConcatLists(list, arg_list);
            list = IRAppendCode(list, code);
            return list;
          }
        }
      }
      break;
    }
    case INT: {
      IRCode *code = IRNewCode(IR_CODE_ASSIGN);
      IRCodeList list = {code, code};
      code->assign.left = place;
      code->assign.right = IRNewConstantOperand(e1->ival);
      return IRWrapCode(code);
    }
    case FLOAT:
      Panic("unexpected FLOAT");
      break;
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

          IRCode *code1 = IRNewCode(IR_CODE_ASSIGN);
          code1->assign.left = var;
          code1->assign.right = t1;

          IRCode *code2 = IRNewCode(IR_CODE_ASSIGN);
          code2->assign.left = place;
          code2->assign.right = var;

          list = IRAppendCode(list, code1);
          list = IRAppendCode(list, code2);
          return list;
        }
        case AND:
        case OR:
        case RELOP:
          return IRTranslateCondPre(exp, place);
        default: {
          IROperand t1 = IRNewTempOperand();
          IROperand t2 = IRNewTempOperand();
          IRCodeList list1 = IRTranslateExp(e1, t1);
          IRCodeList list2 = IRTranslateExp(e3, t2);

          IRCode *code = NULL;
          switch (e2->token) {
            case PLUS:
              code = IRNewCode(IR_CODE_ADD);
              break;
            case MINUS:
              code = IRNewCode(IR_CODE_SUB);
              break;
            case STAR:  // not MUL
              code = IRNewCode(IR_CODE_MUL);
              break;
            case DIV:
              code = IRNewCode(IR_CODE_DIV);
              break;
            default:
              Panic("invalid arithmic code");
          }
          code->binop.result = place;
          code->binop.op1 = t1;
          code->binop.op2 = t2;

          IRCodeList list = IRConcatLists(list1, list2);
          return IRAppendCode(list, code);
        }
      }
    }
  }
  Panic("should not reach here");
  return STATIC_EMPTY_IR_LIST;
}

// Prepare to translate an Cond Exp.
IRCodeList IRTranslateCondPre(STNode *exp, IROperand place) {
  AssertSTNode(exp, "Exp");
  IROperand l1 = IRNewLabelOperand();
  IROperand l2 = IRNewLabelOperand();

  IRCode *code0 = IRNewCode(IR_CODE_ASSIGN);
  code0->assign.left = place;
  code0->assign.right = IRNewConstantOperand(0);

  IRCodeList list = IRTranslateCond(exp, l1, l2);

  IRCode *code1 = IRNewCode(IR_CODE_LABEL);
  code1->label.label = l1;

  IRCode *code2 = IRNewCode(IR_CODE_ASSIGN);
  code2->assign.left = place;
  code2->assign.right = IRNewConstantOperand(1);

  list = IRConcatLists(IRWrapCode(code0), list);
  list = IRAppendCode(list, code1);
  list = IRAppendCode(list, code2);
  return list;
}

// Translate an Exp into an conditional IRCodeList.
IRCodeList IRTranslateCond(STNode *exp, IROperand label_true, IROperand label_false) {
  AssertSTNode(exp, "Exp");
  if (exp->child->token == NOT) {
    // NOT Exp
    return IRTranslateCond(exp->child->next, label_false, label_true);
  } else if (exp->child->next != NULL) {
    Assert(exp->child->next->next != NULL, "invalid cond format");
    STNode *exp1 = exp->child;
    STNode *exp2 = exp1->next->next;
    switch (exp1->next->token) {
      case RELOP: {
        // Exp1 RELOP Exp2
        IROperand t1 = IRNewTempOperand();
        IROperand t2 = IRNewTempOperand();

        IRCodeList list = IRTranslateExp(exp1, t1);
        IRCodeList list2 = IRTranslateExp(exp2, t2);
        list = IRConcatLists(list, list2);

        IRCode *jump1 = IRNewCode(IR_CODE_JUMP_COND);
        jump1->jump_cond.op1 = t1;
        jump1->jump_cond.op2 = t2;
        jump1->jump_cond.relop = IRNewRelopOperand(exp1->next->rval);
        jump1->jump_cond.dest = label_true;
        list = IRAppendCode(list, jump1);

        IRCode *jump2 = IRNewCode(IR_CODE_JUMP);
        jump2->jump.dest = label_false;
        list = IRAppendCode(list, jump2);
        return list;
      }
      case AND: {
        // Exp1 AND Exp2
        IROperand l1 = IRNewLabelOperand();
        
        IRCodeList list = IRTranslateCond(exp1, l1, label_false);
        IRCode *label = IRNewCode(IR_CODE_LABEL);
        label->label.label = l1;
        list = IRAppendCode(list, label);

        IRCodeList list2 = IRTranslateCond(exp2, label_true, label_false);
        list = IRConcatLists(list, list2);
        return list;
      }
      case OR: {
        // Exp1 OR Exp2
        IROperand l1 = IRNewLabelOperand();
        
        IRCodeList list = IRTranslateCond(exp1, label_true, l1);
        IRCode *label = IRNewCode(IR_CODE_LABEL);
        label->label.label = l1;
        list = IRAppendCode(list, label);

        IRCodeList list2 = IRTranslateCond(exp2, label_true, label_false);
        list = IRConcatLists(list, list2);
        return list;
      }
      default:
        Panic("unknown condition format");
    }
  } else {
    // Exp (like if(0), while(1))
    IROperand t1 = IRNewTempOperand();
    IRCodeList list = IRTranslateExp(exp->child, t1);

    IRCode *jump = IRNewCode(IR_CODE_JUMP_COND);
    jump->jump_cond.op1 = t1;
    jump->jump_cond.op2 = IRNewConstantOperand(0);
    jump->jump_cond.relop = IRNewRelopOperand(RELOP_NE);
    jump->jump_cond.dest = label_true;
    list = IRAppendCode(list, jump);

    IRCode *label = IRNewCode(IR_CODE_LABEL);
    label->label.label = label_false;
    list = IRAppendCode(list, label);
    return list;
  }
  return STATIC_EMPTY_IR_LIST;
}

// Translate an CompSt into an IRCodeList.
IRCodeList IRTranslateCompSt(STNode *comp) {
  AssertSTNode(comp, "CompSt");
  Panic("not implemented");
  return STATIC_EMPTY_IR_LIST;
}

// Translate an Stmt into an IRCodeList.
IRCodeList IRTranslateStmt(STNode *stmt) {
  AssertSTNode(stmt, "Stmt");
  if (stmt->child->next == NULL) {
    // As we exit CompSt, symbol table is destroyed.
    // Therefore, we first translate inner codes and push them into IR queue.
    // When translating the higher-level codes, we pop the queue and get the code.
    Assert(!IRQueueEmpty(), "IR queue empty when translating Stmt");
    return IRQueuePop();  // FIFO queue
  } else {
    switch (stmt->child->token) {
      case RETURN: { // RETURN Exp SEMI
        IROperand t1 = IRNewTempOperand();
        IRCodeList list = IRTranslateExp(stmt->child->next, t1);
        IRCode *code = IRNewCode(IR_CODE_RETURN);
        code->ret.value = t1;

        return IRAppendCode(list, code);
      }
      case IF: { // IF LP Exp RP Stmt [ELSE Stmt]
        STNode *exp = stmt->child->next->next;
        STNode *stmt1 = exp->next->next;
        STNode *stmt2 = stmt1->next ? stmt1->next->next : NULL;

        IROperand l1 = IRNewLabelOperand();
        IROperand l2 = IRNewLabelOperand();

        IRCode *label1 = IRNewCode(IR_CODE_LABEL);
        label1->label.label = l1;
        IRCode *label2 = IRNewCode(IR_CODE_LABEL);
        label2->label.label = l2;

        IRCodeList list = IRTranslateCond(exp, l1, l2);
        IRCodeList list1 = IRTranslateStmt(stmt1);
        list = IRAppendCode(list, label1);
        list = IRConcatLists(list, list1);

        if (stmt2 == NULL) {
          list = IRAppendCode(list, label2);
        } else {
          IROperand l3 = IRNewLabelOperand();
          IRCode *jump = IRNewCode(IR_CODE_JUMP);
          jump->jump.dest = l3;

          list = IRAppendCode(list, jump);
          list = IRAppendCode(list, label2);
          
          IRCodeList list2 = IRTranslateStmt(stmt2);
          list = IRConcatLists(list, list2);

          IRCode *label3 = IRNewCode(IR_CODE_LABEL);
          label3->label.label = l3;
          list = IRAppendCode(list, label3);
        }
        return list;
      }
      case WHILE: { // WHILE LP Exp RP Stmt
        STNode *exp = stmt->child->next->next;
        STNode *body = exp->next->next;

        IROperand l1 = IRNewLabelOperand();
        IROperand l2 = IRNewLabelOperand();
        IROperand l3 = IRNewLabelOperand();

        IRCode *label1 = IRNewCode(IR_CODE_LABEL);
        IRCode *label2 = IRNewCode(IR_CODE_LABEL);
        IRCode *label3 = IRNewCode(IR_CODE_LABEL);

        label1->label.label = l1;
        label2->label.label = l2;
        label3->label.label = l3;

        IRCodeList list = IRWrapCode(label1);
        IRCodeList cond = IRTranslateCond(exp, l2, l3);
        list = IRConcatLists(list, cond);
        list = IRAppendCode(list, label2);

        IRCodeList code = IRTranslateStmt(body);
        list = IRConcatLists(list, code);

        IRCode *jump = IRNewCode(IR_CODE_JUMP);
        jump->jump.dest = l1;
        list = IRAppendCode(list, jump);
        list = IRAppendCode(list, label3);
        return list;
      }
      default: { // Exp SEMI
        return IRTranslateExp(stmt->child, IRNewNullOperand());
      }
    }
  }
  Panic("should not reach here");
  return STATIC_EMPTY_IR_LIST;
}

// Translate an Args into an IRCodeList.
IRCodeList IRTranslateArgs(STNode *args, IRCodeList *arg_list) {
  AssertSTNode(args, "Args");
  STNode *exp = args->child;
  IROperand t1 = IRNewTempOperand();
  IRCodeList list = IRTranslateExp(exp, t1);
  
  IRCode *code = IRNewCode(IR_CODE_ARG);
  code->arg.variable = t1;
  *arg_list = IRConcatLists(IRWrapCode(code), *arg_list);
  
  if (exp->next) {
    list = IRConcatLists(list, IRTranslateArgs(exp->next->next, arg_list));
  }
  return list;
}

// This function is unique.
// All codes are translated as CompSt and pushed into IR queue.
// Therefore we only need to add a function, pop the code from queue,
// and link all new codes to the global IR list.
extern IRCodeList irlist; // defined in main.c
void IRTranslateFunc(const char *name) {
  Assert(!IRQueueEmpty(), "IR queue empty when adding func");
  IRCode *code = IRNewCode(IR_CODE_FUNCTION);
  code->function.function.kind = IR_OP_FUNCTION;
  code->function.function.name = name;
  irlist = IRAppendCode(irlist, code);
  irlist = IRConcatLists(irlist, IRQueuePop());
}

// Allocate a new null operand.
IROperand IRNewNullOperand() {
  IROperand op;
  op.kind = IR_OP_NULL;
  return op;
}

// Allocate a new temporary operand.
IROperand IRNewTempOperand() {
  IROperand op;
  op.kind = IR_OP_TEMP;
  op.number = IRTempNumber++;
  return op;
}

// Allocate a new label operand.
IROperand IRNewLabelOperand() {
  IROperand op;
  op.kind = IR_OP_LABEL;
  op.number = IRLabelNumber++;
  return op;
}

// Generate a new variable operand from STNode.
IROperand IRNewVariableOperand(STNode *id) {
  AssertSTNode(id, "ID");
  IROperand op;
  op.kind = IR_OP_VARIABLE;
  op.name = id->sval;
  return op;
}

// Generate a new constant operand.
IROperand IRNewConstantOperand(int value) {
  IROperand op;
  op.kind = IR_OP_CONSTANT;
  op.ivalue = value;
  return op;
}

// Generate a new relation-operator operand.
IROperand IRNewRelopOperand(enum ENUM_RELOP relop) {
  IROperand op;
  op.kind = IR_OP_RELOP;
  op.relop = relop;
  return op;
}

// Generate a new function operand.
IROperand IRNewFunctionOperand(const char *name) {
  IROperand op;
  op.kind = IR_OP_FUNCTION;
  op.name = name;
  return op;
}

// Parse an operand to string
size_t IRParseOperand(char *s, IROperand *op) {
  switch (op->kind) {
    case IR_OP_TEMP:
      return sprintf(s, "_IR_T%d", op->number);
    case IR_OP_LABEL:
      return sprintf(s, "_IR_L%d", op->number);
    case IR_OP_RELOP: {
      switch (op->relop) {
        case RELOP_EQ:
          return sprintf(s, "==");
        case RELOP_NE:
          return sprintf(s, "!=");
        case RELOP_LT:
          return sprintf(s, "<");
        case RELOP_LE:
          return sprintf(s, "<=");
        case RELOP_GT:
          return sprintf(s, ">");
        default: // RELOP_GE
          return sprintf(s, ">=");
      }
    }
    case IR_OP_VARIABLE:
      return sprintf(s, "%s", op->name);
    case IR_OP_ADDRESS:
      return sprintf(s, "ADDRESS"); // TODO
    case IR_OP_CONSTANT:
      return sprintf(s, "#%d", op->ivalue);
    case IR_OP_FUNCTION:
      return sprintf(s, "%s", op->name);
    default:
      return sprintf(s, "(NULL)");
  }
}

// Parse an IR code to string.
size_t IRParseCode(char *s, IRCode *code) {
  char *beg = s;
  switch (code->kind) {
    case IR_CODE_LABEL: {
      s += sprintf(s, "LABEL ");
      s += IRParseOperand(s, &code->label.label);
      s += sprintf(s, " :");
      break;
    }
    case IR_CODE_FUNCTION: {
      s += sprintf(s, "FUNCTION ");
      s += IRParseOperand(s, &code->function.function);
      s += sprintf(s, " :");
      break;
    }
    case IR_CODE_ASSIGN: {
      s += IRParseOperand(s, &code->assign.left);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->assign.right);
      break;
    }
    case IR_CODE_ADD: {
      s += IRParseOperand(s, &code->binop.result);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->binop.op1);
      s += sprintf(s, " + ");
      s += IRParseOperand(s, &code->binop.op2);
      break;
    }
    case IR_CODE_SUB: {
      s += IRParseOperand(s, &code->binop.result);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->binop.op1);
      s += sprintf(s, " - ");
      s += IRParseOperand(s, &code->binop.op2);
      break;
    }
    case IR_CODE_MUL: {
      s += IRParseOperand(s, &code->binop.result);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->binop.op1);
      s += sprintf(s, " * ");
      s += IRParseOperand(s, &code->binop.op2);
      break;
    }
    case IR_CODE_DIV: {
      s += IRParseOperand(s, &code->binop.result);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->binop.op1);
      s += sprintf(s, " / ");
      s += IRParseOperand(s, &code->binop.op2);
      break;
    }
    case IR_CODE_JUMP: {
      s += sprintf(s, "GOTO ");
      s += IRParseOperand(s, &code->jump.dest);
      break;
    }
    case IR_CODE_JUMP_COND: {
      s += sprintf(s, "IF ");
      s += IRParseOperand(s, &code->jump_cond.op1);
      s += sprintf(s, " ");
      s += IRParseOperand(s, &code->jump_cond.relop);
      s += sprintf(s, " ");
      s += IRParseOperand(s, &code->jump_cond.op2);
      s += sprintf(s, " GOTO ");
      s += IRParseOperand(s, &code->jump_cond.dest);
      break;
    }
    case IR_CODE_RETURN: {
      s += sprintf(s, "RETURN ");
      s += IRParseOperand(s, &code->ret.value);
      break;
    }
    case IR_CODE_DEC: {
      Panic("not implemented");
    }
    case IR_CODE_ARG: {
      s += sprintf(s, "ARG ");
      s += IRParseOperand(s, &code->arg.variable);
      break;
    }
    case IR_CODE_CALL: {
      s += IRParseOperand(s, &code->call.result);
      s += sprintf(s, " := CALL ");
      s += IRParseOperand(s, &code->call.function);
      break;
    }
    case IR_CODE_PARAM: {
      Panic("not implemented");
    }
    case IR_CODE_READ: {
      s += sprintf(s, "READ ");
      s += IRParseOperand(s, &code->read.variable);
      break;
    }
    case IR_CODE_WRITE: {
      s += sprintf(s, "WRITE ");
      s += IRParseOperand(s, &code->write.variable);
      break;
    }
    default:
      Panic("should not reach here");
  }
  return s - beg;
}

// Parse and output a line of IR code to file.
static char ir_buffer[512] = {};
size_t IRWriteCode(int fd, IRCode *code) {
  return write(fd, ir_buffer, IRParseCode(ir_buffer, code));
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
  IRCodeList ret = {list.head, list.tail};
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

// IR queue: save IR of inner CompSts.
static IRQueueItem *IR_QUEUE_HEAD = NULL;
static IRQueueItem *IR_QUEUE_TAIL = NULL;

// Check if the IR queue is empty.
bool IRQueueEmpty() { return IR_QUEUE_HEAD == NULL; }

// Push an IR list into the IR queue.
void IRQueuePush(IRCodeList list) {
  IRQueueItem *item = (IRQueueItem *)malloc(sizeof(IRQueueItem));
  item->list = list;
  item->prev = item->next = NULL;
  if (IRQueueEmpty()) {
    IR_QUEUE_HEAD = IR_QUEUE_TAIL = item;
  } else {
    IR_QUEUE_TAIL->next = item;
    item->prev = IR_QUEUE_TAIL;
    IR_QUEUE_TAIL = item;
  }
}

// Pop an IR list from the IR queue.
IRCodeList IRQueuePop() {
  Assert(!IRQueueEmpty(), "IR queue empty when popping");
  IRCodeList list = IR_QUEUE_HEAD->list;
  IRQueueItem *item = IR_QUEUE_HEAD;
  IR_QUEUE_HEAD = IR_QUEUE_HEAD->next;
  if (IR_QUEUE_HEAD == NULL) {
    IR_QUEUE_TAIL = NULL;
  }
  free(item);  // free the IRQueue item.
  return list;
}

// Clear the IR queue.
void IRQueueClear() {
  while (!IRQueueEmpty()) {
    IRQueuePop();
  }
}
