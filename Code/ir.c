#include "ir.h"

#include <stdbool.h>
#include <unistd.h>

#include "debug.h"
#include "syntax.tab.h"
#include "table.h"
#include "token.h"
#include "tree.h"

// same assertion code as in type.c
#ifdef DEBUG
#define AssertSTNode(node, str) \
  Assert(node, "node is null"); \
  Assert(!strcmp(node->name, str), "not a " str);
#else
#define AssertSTNode(node, str)
#endif

extern SEType *STATIC_TYPE_VOID, *STATIC_TYPE_INT, *STATIC_TYPE_FLOAT;

const IRCodeList STATIC_EMPTY_IR_LIST = {NULL, NULL};

// Translate an Exp into IRCodeList with SEType as a pair.
IRCodePair IRTranslateExp(STNode *exp, IROperand place, bool deref) {
  AssertSTNode(exp, "Exp");
  STNode *e1 = exp->child;
  STNode *e2 = e1 ? e1->next : NULL;
  STNode *e3 = e2 ? e2->next : NULL;
  switch (e1->token) {
    case LP:  // LP Exp RP
      return IRTranslateExp(e2, place, deref);
    case MINUS: {
      IROperand t1 = IRNewTempOperand();
      IRCodePair pair = IRTranslateExp(e2, t1, true);

      if (place.kind != IR_OP_NULL) {
        IRCode *code = IRNewCode(IR_CODE_SUB);
        code->binop.result = place;
        code->binop.op1 = IRNewConstantOperand(0);
        code->binop.op2 = t1;
        pair.list = IRAppendCode(pair.list, code);
      }
      return pair;
    }
    case NOT:
      return IRWrapPair(IRTranslateCondPre(exp, place), STATIC_TYPE_INT, false);
    case ID: {
      STEntry *entry = STSearch(e1->sval);
      Assert(entry != NULL, "entry %s not found in ST", exp->sval);
      SEType *type = entry->type;
      if (e2 == NULL) {
        if (place.kind != IR_OP_NULL) {
          IROperand var = IRNewVariableOperand(e1->sval);
          IRCode *code = IRNewCode(var.kind == IR_OP_MEMBLOCK ? IR_CODE_DEREF : IR_CODE_ASSIGN);
          code->assign.left = place;
          code->assign.right = var;
          return IRWrapPair(IRWrapCode(code), type, type->kind != BASIC);
        } else {
          return IRWrapPair(STATIC_EMPTY_IR_LIST, type, false);
        }
      } else {
        // function calls can't be ignored as they may have side effects!
        // if place is empty, we need to create a temp variable.
        if (place.kind == IR_OP_NULL) {
          place = IRNewTempOperand();
        }
        if (e3->token == RP) {
          // ID()
          if (!strcmp(e1->sval, "read")) {
            IRCode *code = IRNewCode(IR_CODE_READ);
            code->read.variable = place;
            return IRWrapPair(IRWrapCode(code), STATIC_TYPE_INT, false);
          } else {
            IRCode *code = IRNewCode(IR_CODE_CALL);
            code->call.result = place;
            code->call.function = IRNewFunctionOperand(e1->sval);
            return IRWrapPair(IRWrapCode(code), type->function.type, type->function.type->kind != BASIC);
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
            return IRWrapPair(list, STATIC_TYPE_INT, false);
          } else {
            IRCode *code = IRNewCode(IR_CODE_CALL);
            code->call.result = place;
            code->call.function = IRNewFunctionOperand(e1->sval);
            list = IRConcatLists(list, arg_list);
            list = IRAppendCode(list, code);
            return IRWrapPair(list, type->function.type, type->function.type->kind != BASIC);
          }
        }
      }
      break;
    }
    case INT: {
      if (place.kind != IR_OP_NULL) {
        IRCode *code = IRNewCode(IR_CODE_ASSIGN);
        IRCodeList list = {code, code};
        code->assign.left = place;
        code->assign.right = IRNewConstantOperand(e1->ival);
        return IRWrapPair(IRWrapCode(code), STATIC_TYPE_INT, false);
      } else {
        return IRWrapPair(STATIC_EMPTY_IR_LIST, STATIC_TYPE_INT, false);
      }
    }
    case FLOAT:
      Panic("unexpected FLOAT");
      break;
    default: {
      switch (e2->token) {
        case LB: {
          IRCodePair pair = IRTranslateExp(e1, place, false);
          IRCodeList list = pair.list;
          SEType *type = pair.type;
          Assert(type->kind = ARRAY, "type is not array");

          IROperand t1 = IRNewTempOperand();
          IRCodePair pair2 = IRTranslateExp(e3, t1, true);
          list = IRConcatLists(list, pair2.list);

          IRCode *code = IRNewCode(IR_CODE_MUL);
          code->binop.result = t1;
          code->binop.op1 = t1;
          code->binop.op2 = IRNewConstantOperand(type->array.type->size);
          list = IRAppendCode(list, code);

          code = IRNewCode(IR_CODE_ADD);
          code->binop.result = place;
          code->binop.op1 = place;
          code->binop.op2 = t1;
          list = IRAppendCode(list, code);

          if (deref) {
            code = IRNewCode(IR_CODE_LOAD);
            code->load.left = place;
            code->load.right = place;
            list = IRAppendCode(list, code);
          }
          return IRWrapPair(list, type, !deref);
        }
        case DOT: {
          IRCodePair pair = IRTranslateExp(e1, place, false);
          IRCodeList list = pair.list;
          SEType *type = pair.type;
          SEField *field = NULL;
          size_t offset = 0;
          Assert(type->kind = STRUCTURE, "type is not structure");
          for (field = type->structure; field != NULL; field = field->next) {
            if (!strcmp(field->name, e3->sval)) {
              type = field->type;
              break;
            } else {
              offset += field->type->size;
            }
          }
          Assert(field != NULL, "invalid offset in struct");
          if (offset > 0) {
            IRCode *code = IRNewCode(IR_CODE_ADD);
            code->binop.result = place;
            code->binop.op1 = place;
            code->binop.op2 = IRNewConstantOperand(offset);
            list = IRAppendCode(list, code);
          }

          if (deref) {
            IRCode *code = IRNewCode(IR_CODE_LOAD);
            code->load.left = place;
            code->load.right = place;
            list = IRAppendCode(list, code);
          }
          return IRWrapPair(list, type, !deref);
        }
        case ASSIGNOP: {
          IROperand t1 = IRNewTempOperand();
          IRCodePair pair = IRTranslateExp(e3, t1, true);

          IROperand var = IRNewNullOperand();
          IRCode *code1 = NULL;
          if (e1->child->token == ID) {
            // assign to local variable
            var = IRNewVariableOperand(e1->child->sval);
            code1 = IRNewCode(IR_CODE_ASSIGN);
            code1->assign.left = var;
            code1->assign.right = t1;
          } else {
            // assign to address (array/struct)
            var = IRNewTempOperand();
            pair.list = IRConcatLists(pair.list, IRTranslateExp(e1, var, false).list);
            code1 = IRNewCode(IR_CODE_SAVE);
            code1->save.left = var;
            code1->save.right = t1;
          }
          pair.list = IRAppendCode(pair.list, code1);
          if (place.kind != IR_OP_NULL) {
            IRCode *code2 = IRNewCode(IR_CODE_ASSIGN);
            code2->assign.left = place;
            code2->assign.right = var;
            pair.list = IRAppendCode(pair.list, code2);
          }
          return pair;
        }
        case AND:
        case OR:
        case RELOP:
          return IRWrapPair(IRTranslateCondPre(exp, place), STATIC_TYPE_INT, false);
        default: {
          IROperand t1 = IRNewTempOperand();
          IROperand t2 = IRNewTempOperand();
          IRCodePair pair = IRTranslateExp(e1, t1, true);
          IRCodePair pair2 = IRTranslateExp(e3, t2, true);
          pair.list = IRConcatLists(pair.list, pair2.list);

          if (place.kind != IR_OP_NULL) {
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
            pair.list = IRAppendCode(pair.list, code);
          }
          return pair;
        }
      }
    }
  }
  Panic("should not reach here");
  return IRWrapPair(STATIC_EMPTY_IR_LIST, STATIC_TYPE_INT, false);
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
IRCodeList IRTranslateCond(STNode *exp, IROperand label_true,
                           IROperand label_false) {
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

        IRCodePair pair = IRTranslateExp(exp1, t1, true);
        IRCodePair pair2 = IRTranslateExp(exp2, t2, true);
        IRCodeList list = IRConcatLists(pair.list, pair2.list);

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
    IRCodeList list = IRTranslateExp(exp, t1, true).list;

    IRCode *jump = IRNewCode(IR_CODE_JUMP_COND);
    jump->jump_cond.op1 = t1;
    jump->jump_cond.op2 = IRNewConstantOperand(0);
    jump->jump_cond.relop = IRNewRelopOperand(RELOP_NE);
    jump->jump_cond.dest = label_true;
    list = IRAppendCode(list, jump);

    jump = IRNewCode(IR_CODE_JUMP);
    jump->jump.dest = label_false;
    list = IRAppendCode(list, jump);
    return list;
  }
  return STATIC_EMPTY_IR_LIST;
}

// Translate an CompSt into an IRCodeList.
IRCodeList IRTranslateCompSt(STNode *comp) {
  AssertSTNode(comp, "CompSt");
  IRCodeList list = IRTranslateDefList(comp->child->next);
  return IRConcatLists(list, IRTranslateStmtList(comp->child->next->next));
}

// Translate a DefList into an IRCodeList.
IRCodeList IRTranslateDefList(STNode *list) {
  AssertSTNode(list, "DefList");
  if (list->empty) return STATIC_EMPTY_IR_LIST;
  IRCodeList ret = IRTranslateDef(list->child);
  return IRConcatLists(ret, IRTranslateDefList(list->child->next));
}

// Translate a Def into an IRCodeList.
IRCodeList IRTranslateDef(STNode *def) {
  AssertSTNode(def, "Def");
  return IRTranslateDecList(def->child->next);
}

// Translate a DecList into an IRCodeList.
IRCodeList IRTranslateDecList(STNode *list) {
  AssertSTNode(list, "DecList");
  IRCodeList ret = IRTranslateDec(list->child);
  if (list->child->next != NULL) {
    ret = IRConcatLists(ret, IRTranslateDecList(list->child->next->next));
  }
  return ret;
}

// Translate a Dec into an IRCodeList.
IRCodeList IRTranslateDec(STNode *dec) {
  AssertSTNode(dec, "Dec");
  IRCodeList list = STATIC_EMPTY_IR_LIST;

  // find name of variable and get IR number
  STNode *var = dec->child;
  while (var && var->token != ID) {
    var = var->child;
  }
  Assert(var, "no ID inside VarDec");
  IROperand v = IRNewVariableOperand(var->sval);

  // check whether we need DEC an array or a struct (local variable)
  STEntry *entry = STSearchCurr(var->sval);
  Assert(entry, "entry %s not found in ST", var->sval);
  if (entry->type->kind == ARRAY || entry->type->kind == STRUCTURE) {
    Assert(v.kind == IR_OP_MEMBLOCK, "not declaring a memblock");
    IRCode *code = IRNewCode(IR_CODE_DEC);
    code->dec.variable = v;
    code->dec.size = IRNewConstantOperand(entry->type->size);
    list = IRAppendCode(list, code);
  }

  // check whether there is an assignment
  if (dec->child->next != NULL) {
    IROperand t1 = IRNewTempOperand();
    list = IRConcatLists(list, IRTranslateExp(dec->child->next->next, t1, true).list);

    IRCode *code = IRNewCode(IR_CODE_ASSIGN);
    code->assign.left = v;
    code->assign.right = t1;
    list = IRAppendCode(list, code);
  }
  return list;
}

// Translate an StmtList into an IRCodeList.
IRCodeList IRTranslateStmtList(STNode *list) {
  AssertSTNode(list, "StmtList");
  if (list->empty) return STATIC_EMPTY_IR_LIST;
  IRCodeList ret = IRTranslateStmt(list->child);
  return IRConcatLists(ret, IRTranslateStmtList(list->child->next));
}

// Translate an Stmt into an IRCodeList.
IRCodeList IRTranslateStmt(STNode *stmt) {
  AssertSTNode(stmt, "Stmt");
  if (stmt->child->next == NULL) {
    // As we exit CompSt, symbol table is destroyed.
    // Therefore, we first translate inner codes and push them into IR queue.
    // When translating the higher-level codes, we pop the queue and get the
    // code.
    Assert(!IRQueueEmpty(), "IR queue empty when translating Stmt");
    return IRQueuePop();  // FIFO queue
  } else {
    switch (stmt->child->token) {
      case RETURN: {  // RETURN Exp SEMI
        IROperand t1 = IRNewTempOperand();
        IRCodeList list = IRTranslateExp(stmt->child->next, t1, true).list;
        IRCode *code = IRNewCode(IR_CODE_RETURN);
        code->ret.value = t1;

        return IRAppendCode(list, code);
      }
      case IF: {  // IF LP Exp RP Stmt [ELSE Stmt]
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
      case WHILE: {  // WHILE LP Exp RP Stmt
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
      default: {  // Exp SEMI
        return IRTranslateExp(stmt->child, IRNewNullOperand(), true).list;
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
  IRCodeList list = IRTranslateExp(exp, t1, true).list;

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
extern IRCodeList irlist;  // defined in main.c
void IRTranslateFunc(const char *name) {
  Assert(!IRQueueEmpty(), "IR queue empty when adding func");
  
  // Add declaration of function
  IRCode *code = IRNewCode(IR_CODE_FUNCTION);
  code->function.function.kind = IR_OP_FUNCTION;
  code->function.function.name = name;
  irlist = IRAppendCode(irlist, code);
  
  // Traverse all parameters of the function
  STEntry *entry = STSearchFunc(name);
  Assert(entry != NULL, "func %s not found in ST", name);
  Assert(entry->type->kind == FUNCTION, "type is not func");
  for (SEField *field = entry->type->function.signature; field; field = field->next) {
    if (field->kind == VOID) break;
    code = IRNewCode(IR_CODE_PARAM);
    code->param.variable = IRNewVariableOperand(field->name);
    irlist = IRAppendCode(irlist, code);
  }

  // Concat the list of function body (stored in IR queue)
  irlist = IRConcatLists(irlist, IRQueuePop());
}

// Allocate a new null operand.
IROperand IRNewNullOperand() {
  IROperand op;
  op.kind = IR_OP_NULL;
  return op;
}

// Allocate a new temporary operand.
static unsigned int IRTempNumber = 0;
IROperand IRNewTempOperand() {
  IROperand op;
  op.kind = IR_OP_TEMP;
  op.number = ++IRTempNumber;
  return op;
}

// Allocate a new label operand.
static unsigned int IRLabelNumber = 0;
IROperand IRNewLabelOperand() {
  IROperand op;
  op.kind = IR_OP_LABEL;
  op.number = ++IRLabelNumber;
  return op;
}

// Generate a new variable operand from STNode.
static unsigned int IRVariableNumber = 0;
IROperand IRNewVariableOperand(const char *name) {
  STEntry *entry = STSearch(name);
  Assert(entry != NULL, "entry %s not found in ST", name);
  Assert(entry->number >= 0, "invalid entry number %d", entry->number);
  if (entry->number == 0) {
    entry->number = ++IRVariableNumber;
  }

  IROperand op;
  if (entry->type->kind != BASIC) {
    op.kind = entry->allocate ? IR_OP_MEMBLOCK : IR_OP_VADDRESS;
  } else {
    op.kind = IR_OP_VARIABLE;
  }
  Log("%s: %s", name, (op.kind == IR_OP_MEMBLOCK ? "MEM" : (op.kind == IR_OP_VADDRESS ? "ADD" : "VAR")));
  op.number = entry->number;
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
      return sprintf(s, "t%u", op->number);
    case IR_OP_LABEL:
      return sprintf(s, "label%u", op->number);
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
        default:  // RELOP_GE
          return sprintf(s, ">=");
      }
    }
    case IR_OP_VARIABLE:
    case IR_OP_VADDRESS:
    case IR_OP_MEMBLOCK:
      return sprintf(s, "v%u", op->number);
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
    case IR_CODE_ADD:
    case IR_CODE_SUB:
    case IR_CODE_MUL:
    case IR_CODE_DIV: {
      char op = ' ';
      switch (code->kind) {
        case IR_CODE_ADD:
          op = '+'; break;
        case IR_CODE_SUB:
          op = '-'; break;
        case IR_CODE_MUL:
          op = '*'; break;
        case IR_CODE_DIV:
          op = '/'; break;
        default:
          Panic("invalid arithmic operand");
      }
      s += IRParseOperand(s, &code->binop.result);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->binop.op1);
      s += sprintf(s, " %c ", op);
      s += IRParseOperand(s, &code->binop.op2);
      break;
    }
    case IR_CODE_DEREF: {
      s += IRParseOperand(s, &code->addr.left);
      s += sprintf(s, " := &");
      s += IRParseOperand(s, &code->addr.right);
      break;
    }
    case IR_CODE_LOAD: {
      s += IRParseOperand(s, &code->load.left);
      s += sprintf(s, " := *");
      s += IRParseOperand(s, &code->load.right);
      break;
    }
    case IR_CODE_SAVE: {
      s += sprintf(s, "*");
      s += IRParseOperand(s, &code->save.left);
      s += sprintf(s, " := ");
      s += IRParseOperand(s, &code->save.right);
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
      Assert(code->dec.size.kind == IR_OP_CONSTANT, "size if not constant");
      s += sprintf(s, "DEC ");
      s += IRParseOperand(s, &code->dec.variable);
      s += sprintf(s, " %u", code->dec.size.ivalue);  // special case
      break;
    }
    case IR_CODE_ARG: {
      s += sprintf(s, "ARG ");
      s += IRParseOperand(s, &code->arg.variable);
      break;
    }
    case IR_CODE_PARAM: {
      s += sprintf(s, "PARAM ");
      s += IRParseOperand(s, &code->arg.variable);
      break;
    }
    case IR_CODE_CALL: {
      s += IRParseOperand(s, &code->call.result);
      s += sprintf(s, " := CALL ");
      s += IRParseOperand(s, &code->call.function);
      break;
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
size_t IRWriteCode(FILE *f, IRCode *code) {
  IRParseCode(ir_buffer, code);
  return fprintf(f, "%s\n", ir_buffer);
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

// Wrap a code list and a type to IRCodePair.
IRCodePair IRWrapPair(IRCodeList list, SEType *type, bool addr) {
  IRCodePair pair;
  pair.list = list;
  pair.type = type;
  pair.addr = addr;
  return pair;
}

// Append a code to the end of list.
IRCodeList IRAppendCode(IRCodeList list, IRCode *code) {
  Assert(code, "appending null code");
#ifdef DEBUG
  char tmp[256] = {};
  IRParseCode(tmp, code);
  CLog(FG_YELLOW, "%s", tmp);
#endif
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
  if (list2.head == NULL) {
    return list1;  // concat with empty list
  } else if (list1.tail == NULL) {
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
