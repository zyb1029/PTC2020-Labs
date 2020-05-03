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
        Panic("not implemented!");
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
  Panic("not implemented");
  return STATIC_EMPTY_IR_LIST;
}

// Translate an CompSt into an IRCodeList.
IRCodeList IRTranslateCompSt(STNode *compst) {
  AssertSTNode(compst, "CompSt");
  Panic("not implemented");
  return STATIC_EMPTY_IR_LIST;
}

// Translate an Stmt into an IRCodeList.
IRCodeList IRTranslateStmt(STNode *stmt) {
  AssertSTNode(stmt, "Stmt");
  if (stmt->child->next == NULL) {
    // As we exit CompSt, symbol table is destroyed.
    // Therefore, do not make recursive translating on CompSt.
    // FIXME: HOW TO IMPLEMENT THIS?
    return IRTranslateCompSt(stmt->child);
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
