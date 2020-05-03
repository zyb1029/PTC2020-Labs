#include <string.h>
#include "tree.h"
#include "type.h"
#include "table.h"
#include "ir.h"
#include "semantics.h"
#include "syntax.tab.h"
#include "debug.h"

#ifdef DEBUG
#define AssertSTNode(node, str) \
  Assert(node, "node is null"); \
  Assert(!strcmp(node->name, str), "not a " str);
#else
#define AssertSTNode(node, str)
#endif

int anonymous = 0; // anonymous structure counter
SEType _STATIC_TYPE_VOID, _STATIC_TYPE_INT, _STATIC_TYPE_FLOAT;
SEType *STATIC_TYPE_VOID, *STATIC_TYPE_INT, *STATIC_TYPE_FLOAT;
SEField STATIC_FIELD_VOID, STATIC_FIELD_INT, DUMMY_FIELD;
const SEFieldChain DUMMY_FIELD_CHAIN = { &DUMMY_FIELD, &DUMMY_FIELD };

void SEPrepare() {
  // Prepare basic types
  STATIC_TYPE_VOID = &_STATIC_TYPE_VOID;
  STATIC_TYPE_VOID->kind = VOID;
  STATIC_TYPE_VOID->size = -1;
  STATIC_TYPE_VOID->parent = STATIC_TYPE_VOID;
  STATIC_FIELD_VOID.type = STATIC_TYPE_VOID;
  STATIC_FIELD_VOID.next = NULL;
  STATIC_TYPE_INT = &_STATIC_TYPE_INT;
  STATIC_TYPE_INT->kind = BASIC;
  STATIC_TYPE_INT->size = 4;
  STATIC_TYPE_INT->basic = INT;
  STATIC_TYPE_INT->parent = STATIC_TYPE_INT;
  STATIC_FIELD_INT.type = STATIC_TYPE_INT;
  STATIC_FIELD_INT.next = NULL;
  STATIC_TYPE_FLOAT = &_STATIC_TYPE_FLOAT;
  STATIC_TYPE_FLOAT->kind = BASIC;
  STATIC_TYPE_FLOAT->size = 4;
  STATIC_TYPE_FLOAT->basic = FLOAT;
  STATIC_TYPE_FLOAT->parent = STATIC_TYPE_FLOAT;

  // Add READ and WRITE functions
  SEType *readType = (SEType *)malloc(sizeof(SEType));
  readType->kind = FUNCTION;
  readType->function.defined = true;
  readType->function.node = NULL;
  readType->function.type = STATIC_TYPE_INT;
  readType->function.signature = &STATIC_FIELD_VOID;
  STInsertFunc("read", readType);

  SEType *writeType = (SEType *)malloc(sizeof(SEType));
  writeType->kind = FUNCTION;
  writeType->function.defined = true;
  writeType->function.node = NULL;
  writeType->function.type = STATIC_TYPE_INT;
  writeType->function.signature = &STATIC_FIELD_INT;
  STInsertFunc("write", writeType);
}

// Parse an expression. Only one type so we don't need a chain.
#define malloc(s) NO_MALLOC_ALLOWED_EXP(s)
SEType *SEParseExp(STNode *exp) {
  AssertSTNode(exp, "Exp");
  STNode *e1 = exp->child;
  STNode *e2 = e1 ? e1->next : NULL;
  STNode *e3 = e2 ? e2->next : NULL;
  switch (e1->token) {
    case LP: // LP Exp RP
      return SEParseExp(e2);
    case MINUS: {
      CLog(FG_CYAN, "MINUS Exp");
      SEType *type = SEParseExp(e2);
      if (type->kind != BASIC) {
        throwErrorS(SE_MISMATCHED_OPERANDS, e1->line, NULL);
      }
      return type;
    }
    case NOT: {
      CLog(FG_CYAN, "NOT Exp");
      SEType *type = SEParseExp(e2);
      if (!SECompareType(type, STATIC_TYPE_INT)) {
        throwErrorS(SE_MISMATCHED_OPERANDS, e1->line, NULL);
      }
      return type;
    }
    case ID: {
      if (e2 == NULL) {
        STEntry *entry = STSearch(e1->sval);
        if (entry == NULL || STSearchStru(e1->sval) != NULL) {
          // undefined variable or same name as struct, treat as int
          throwErrorS(SE_VARIABLE_UNDEFINED, e1->line, e1->sval);
          return STATIC_TYPE_INT;
        } else {
          return entry->type;
        }
      } else {
        STEntry *entry = NULL;
        SEField *signature = NULL;
        CLog(FG_CYAN, "%s", e3->next ? "ID LP Args RP" : "ID LP RP");
        entry = STSearch(e1->sval);
        if (entry == NULL) {
          // undefined function, treat as int
          throwErrorS(SE_FUNCTION_UNDEFINED, e1->line, e1->sval);
          return STATIC_TYPE_INT;
        } else if (entry->type->kind != FUNCTION) {
          // call to a non-function variable
          throwErrorS(SE_ACCESS_TO_NON_FUNCTION, e1->line, e1->sval);
          return STATIC_TYPE_INT;
        }
        signature = e3->next ? SEParseArgs(e3).head : &STATIC_FIELD_VOID;
        if (!SECompareField(entry->type->function.signature, signature)) {
          throwErrorS(SE_MISMATCHED_SIGNATURE, e1->line, e1->sval);
        }
        return entry->type->function.type;
      }
      break;
    }
    case INT:
    case FLOAT: {
      return e1->token == INT ? STATIC_TYPE_INT : STATIC_TYPE_FLOAT;
    }
    default: {
      SEType *t1 = SEParseExp(e1);
      switch (e2->token) {
        case LB: {
          CLog(FG_CYAN, "Exp LB Exp RB");
          if (t1->kind != ARRAY) {
            throwErrorS(SE_ACCESS_TO_NON_ARRAY, e2->line, NULL);
            return t1;
          }
          SEType *t2 = SEParseExp(e3);
          if (t2->kind != BASIC || t2->basic != INT) {
            throwErrorS(SE_NON_INTEGER_INDEX, e3->line, NULL);
          }
          return t1->array.type;
        }
        case DOT: {
          CLog(FG_CYAN, "Exp DOT ID");
          if (t1->kind != STRUCTURE) {
            throwErrorS(SE_ACCESS_TO_NON_STRUCT, e2->line, NULL);
            return t1;
          } else {
            SEType *type = NULL;
            SEField *field = t1->structure;
            while (field != NULL) {
              if (!strcmp(field->name, e3->sval)) {
                type = field->type;
                break;
              }
              field = field->next;
            }
            if (type == NULL) {
              throwErrorS(SE_STRUCT_FIELD_UNDEFINED, e3->line, e3->sval);
              type = STATIC_TYPE_INT; // treat as INT
            }
            return type;
          }
        }
        case ASSIGNOP: {
          bool lvalue = false;
          SEType *t2 = SEParseExp(e3);
          CLog(FG_CYAN, "Exp ASSIGNOP Exp");
          Log("DUMP LEFT:"); SEDumpType(t1);
          Log("DUMP RIGHT:"); SEDumpType(t2);
          if (!SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_ASSIGNMENT, e2->line, NULL);
          }
          if (e1->child->token == ID) lvalue = e1->child->next == NULL;
          if (!lvalue && e1->child->next) {
            lvalue = e1->child->next->token == LB
                  || e1->child->next->token == DOT;
          }
          if (!lvalue) {
            // Not any of ID / Exp LB Exp RB / Exp DOT ID
            throwErrorS(SE_RVALUE_ASSIGNMENT, e2->line, NULL);
          }
          return t1;
        }
        case AND:
        case OR: {
          SEType *t2 = SEParseExp(e3);
          CLog(FG_CYAN, "Exp AND/OR Exp");
          Log("DUMP LEFT:"); SEDumpType(t1);
          Log("DUMP RIGHT:"); SEDumpType(t2);
          if (!SECompareType(t1, STATIC_TYPE_INT) ||
              !SECompareType(t2, STATIC_TYPE_INT)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2->line, NULL);
          }
          return STATIC_TYPE_INT; // always return INT
        }
        case RELOP: {
          CLog(FG_CYAN, "Exp RELOP Exp");
          SEType *t2 = SEParseExp(e3);
          if (t1->kind != BASIC || !SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2->line, NULL);
          }
          return STATIC_TYPE_INT; // always return INT
        }
        default: {
          CLog(FG_CYAN, "Exp PLUS/MINUS/STAR/DIV Exp");
          SEType *t2 = SEParseExp(e3);
          if (t1->kind != BASIC || !SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2->line, NULL);
          }
          return t1; // always treat as t1
        }
      }
    }
  }
  Panic("should not reach here");
  return NULL;
}
#undef malloc

// Parse a specifier. Only one type so we don't need a chain.
SEType *SEParseSpecifier(STNode *specifier) {
  AssertSTNode(specifier, "Specifier");
  STNode *child = specifier->child;
  if (child->token == TYPE) {
    if (!strcmp(child->sval, "int")) {
      Log("INT");
      return STATIC_TYPE_INT;
    } else {
      Log("FLOAT");
      return STATIC_TYPE_FLOAT;
    }
  } else {
    Log("STRUCT");
    child = child->child; // STRUCT
    Assert(child->token == STRUCT, "child is not struct");
    STNode *tag = child->next;
    if (tag->next) {
      // define a new struct
      // STRUCT OptTag LC DefList RC
      char *name = tag->empty ? NULL : tag->child->sval;
      SEType *type = (SEType *)malloc(sizeof(SEType));
      {
        STPushStack(STACK_STRUCTURE);
        type->extended = true; // struct has global scope
        type->kind = STRUCTURE;
        type->size = 0;
        type->parent = type;
        type->structure = SEParseDefList(tag->next->next, false).head;
        for (SEField *field = type->structure; field; field = field->next) {
          type->size += field->type->size;
        }
        STPopStack();
      }
      if (tag->empty) {
        // ID never begins with a space so it's safe!
        name = (char *)malloc(sizeof(char) * 32);
        sprintf(name, " ANONYMOUS_STRUCT_%08x", anonymous++);
      }
      CLog(FG_GREEN, "new structure \"%s\"", name);
      if (STSearch(name) != NULL) {
        // struct cannot have same name with variable
        throwErrorS(SE_STRUCT_DUPLICATE, tag->line, name);
      } else if (STSearchStru(name) != NULL) {
        throwErrorS(SE_STRUCT_DUPLICATE, tag->line, name);
      } else {
        STInsertStru(name, type);
      }
      return type;
    } else {
      // STRUCT Tag
      const char *name = tag->child->sval;
      STEntry *entry = STSearchStru(name);
      if (entry == NULL) {
        // undefined struct, treat as INT
        throwErrorS(SE_STRUCT_UNDEFINED, tag->line, name);
        return STATIC_TYPE_INT;
      } else if (entry->type->kind != STRUCTURE) {
        // duplicated name of struct, treat as INT
        throwErrorS(SE_STRUCT_DUPLICATE, tag->line, name);
        return STATIC_TYPE_INT;
      } else {
        return entry->type;
      }
    }
  }
  Panic("should not reach here");
  return NULL;
}

// Parse an ext-definition list.
#define malloc(s) NO_MALLOC_ALLOWED_EXT_DEF_LIST(s)
void SEParseExtDefList(STNode *list) {
  AssertSTNode(list, "ExtDefList");
  if (list->empty) return;
  SEParseExtDef(list->child);
  SEParseExtDefList(list->child->next);
}
#undef malloc

// Parse an ext-definition.
#define malloc(s) NO_MALLOC_ALLOWED_EXT_DEF(s)
void SEParseExtDef(STNode *edef) {
  AssertSTNode(edef, "ExtDef");
  SEType *type = SEParseSpecifier(edef->child);
  STNode *body = edef->child->next;
  if (body->token == SEMI) return;
  if (!strcmp(body->name, "ExtDecList")) {
    SEParseExtDecList(body, type);
  } else {
    SEParseFunDec(body, type); // CompSt handled by FunDec
  }
}
#undef malloc

// Parse an ext-declaration list.
#define malloc(s) NO_MALLOC_ALLOWED_EXT_DEC_LIST(s)
void SEParseExtDecList(STNode *list, SEType *type) {
  AssertSTNode(list, "ExtDecList");
  SEParseVarDec(list->child, type, false);
  if (list->child->next) {
    SEParseExtDecList(list->child->next->next, type);
  }
}
#undef malloc

// Parse a function declaration.
void SEParseFunDec(STNode *fdec, SEType *type) {
  AssertSTNode(fdec, "FunDec");
  STNode *id = fdec->child;
  STNode *vars = id->next->next;
  const char *name = id->sval;
  STEntry *entry = STSearchFunc(name);
  SEType *func = NULL;
  SEField *signature = NULL;

  if (entry == NULL) CLog(FG_GREEN, "new function \"%s\"", name);

  STPushStack(STACK_LOCAL); // treat signature as inner scope
  if (vars->next) {
    SEFieldChain chain = SEParseVarList(vars);
    for (SEField *field = chain.head; field != NULL; field = field->next) {
      field->type->extended = true; // signature should not be destroyed locally
    }
    signature = chain.head;
  } else {
    signature = &STATIC_FIELD_VOID;
  }

  if (entry == NULL) {
    func = (SEType *)malloc(sizeof(SEType));
    func->kind = FUNCTION;
    func->size = -1;
    func->parent = func;
    func->function.node = fdec;
    func->function.defined = fdec->next->token != SEMI;
    func->function.type = type;
    func->function.signature = signature;
    STInsertFunc(name, func);
  } else {
    func = entry->type;
    if (func->kind != FUNCTION) {
      // treat as bad function call
      throwErrorS(SE_ACCESS_TO_NON_FUNCTION, id->line, name);
    } else {
      if (fdec->next->token != SEMI) {
        if (func->function.defined) {
          throwErrorS(SE_FUNCTION_DUPLICATE, id->line, name);
        } else {
          CLog(FG_GREEN, "def function \"%s\"", name);
          func->function.defined = true;
        }
      }
      if (!SECompareType(func->function.type, type)) {
        throwErrorS(SE_FUNCTION_CONFLICTING, id->line, name);
      }
      if (!SECompareField(func->function.signature, signature)) {
        throwErrorS(SE_FUNCTION_CONFLICTING, id->line, name);
      }
    }
  }
  if (fdec->next->token != SEMI) {
    SEParseCompSt(fdec->next, type);
  }
  STPopStack();
  // At this moment, the code of the function is in IR queue.
  // We need to pop it from queue and link to the global list.
  IRTranslateFunc(name);
}

// Parse a composed statement list and check for RETURN statements.
#define malloc(s) NO_MALLOC_ALLOWED_COMPST(s)
void SEParseCompSt(STNode *comp, SEType *type) {
  AssertSTNode(comp, "CompSt");
  // We do not need to push/pop stack, that should be done by the CALLER.
  // CompSt -> LC DefList StmtList RC
  SEParseDefList(comp->child->next, true);
  SEParseStmtList(comp->child->next->next, type);
  // After returning, the stack will be destroyed.
  // Therefore we need to conduct a translation and push code to queue.
  IRQueuePush(IRTranslateCompSt(comp));
}
#undef malloc

// Parse a statement list and check for RETURN statements.
#define malloc(s) NO_MALLOC_ALLOWED_STMT_LIST(s)
void SEParseStmtList(STNode *list, SEType *type) {
  AssertSTNode(list, "StmtList");
  if (list->empty) return;
  SEParseStmt(list->child, type);
  SEParseStmtList(list->child->next, type);
}
#undef malloc

// Parse a single statement and check for RETURN statements.
#define malloc(s) NO_MALLOC_ALLOWED_STMT(s)
void SEParseStmt(STNode *stmt, SEType *type) {
  AssertSTNode(stmt, "Stmt");
  if (stmt->child->next == NULL) {
    STPushStack(STACK_LOCAL);
    SEParseCompSt(stmt->child, type);
    STPopStack();
    return;
  } else {
    switch (stmt->child->token) {
      case RETURN: { // RETURN Exp SEMI
        SEType *ret = SEParseExp(stmt->child->next);
        if (!SECompareType(type, ret)) {
          throwErrorS(SE_MISMATCHED_RETURN, stmt->child->line, NULL);
        }
        return;
      }
      case IF: { // IF LP Exp RP Stmt [ELSE Stmt]
        STNode *enode = stmt->child->next->next;
        STNode *snode = enode->next->next;
        SEType *etype = SEParseExp(enode);
        if (!SECompareType(etype, STATIC_TYPE_INT)) {
          throwErrorS(SE_MISMATCHED_OPERANDS, enode->line, NULL);
        }
        SEParseStmt(snode, type);
        if (snode->next) {
          SEParseStmt(snode->next->next, type);
        }
        return;
      }
      case WHILE: { // WHILE LP Exp RP Stmt
        STNode *enode = stmt->child->next->next;
        STNode *snode = enode->next->next;
        SEType *etype = SEParseExp(enode);
        if (!SECompareType(etype, STATIC_TYPE_INT)) {
          throwErrorS(SE_MISMATCHED_OPERANDS, enode->line, NULL);
        }
        SEParseStmt(snode, type);
        return;
      }
      default: { // Exp SEMI
        SEParseExp(stmt->child);
        return;
      }
    }
  }
  Panic("should not reach here");
}
#undef malloc

/**
 * SEFieldChain is a struct of head and tail of the chain.
 *                chain
 *   +-------------+-------------------+
 *   |                                 |
 * head -> head+1 -> ... -> tail-1 -> tail
 *
 * If the node is a struct definition or a function signature,
 * we will want the chain to store its into in ST. Otherwise,
 * we do not care about what the chain contains at all.
 * */
// Parse a definition list. Return a field chain.
#define malloc(s) NO_MALLOC_ALLOWED_DEF_LIST(s)
SEFieldChain SEParseDefList(STNode *list, bool assignable) {
  AssertSTNode(list, "DefList");
  if (list->empty) return DUMMY_FIELD_CHAIN;
  SEFieldChain chain = SEParseDef(list->child, assignable);
  SEFieldChain tail = SEParseDefList(list->child->next, assignable);
  if (tail.head != &DUMMY_FIELD) {
    chain.tail->next = tail.head;
    chain.tail = tail.tail;
  }
  return chain;
}
#undef malloc

// Parse a single definition. Return a field chain.
#define malloc(s) NO_MALLOC_ALLOWED_DEF(s)
SEFieldChain SEParseDef(STNode *def, bool assignable) {
  AssertSTNode(def, "Def");
  SEType *type = SEParseSpecifier(def->child);
  return SEParseDecList(def->child->next, type, assignable);
}
#undef malloc

// Parse a declaration list. Return a field chain.
#define malloc(s) NO_MALLOC_ALLOWED_DEC_LIST(s)
SEFieldChain SEParseDecList(STNode *list, SEType *type, bool assignable) {
  AssertSTNode(list, "DecList");
  SEFieldChain chain = SEParseDec(list->child, type, assignable);
  if (list->child->next) {
    SEFieldChain tail = SEParseDecList(list->child->next->next, type, assignable);
    if (!assignable) {
      chain.tail->next = tail.head;
      chain.tail = tail.tail;
    }
  }
  return chain;
}
#undef malloc

// Parse a single declaration. Return a field chain.
#define malloc(s) NO_MALLOC_ALLOWED_DEC(s)
SEFieldChain SEParseDec(STNode *dec, SEType *type, bool assignable) {
  AssertSTNode(dec, "Dec");
  // We don't care about the chain, but we need the type!!
  SEFieldChain chain = SEParseVarDec(dec->child, type, assignable);
  if (dec->child->next != NULL) { // check assignment
    if (!assignable) {
      STNode *id = dec->child;
      while (id->token != ID) id = id->child;
      throwErrorS(SE_STRUCT_FIELD_INITIALIZED, dec->child->next->line, id->sval);
    }
    SEType *expType = SEParseExp(dec->child->next->next);
    if (!SECompareType(chain.head->type, expType)) {
      throwErrorS(SE_MISMATCHED_ASSIGNMENT, dec->child->next->line, NULL);
    }
  }
  return chain;
}
#undef malloc

// Parse a variable declaration. Return a field chain.
SEFieldChain SEParseVarDec(STNode *var, SEType *type, bool assignable) {
  AssertSTNode(var, "VarDec");
  if (var->child->next) {
    // VarDec LB INT RB
    int arraySize = var->child->next->next->ival;
    SEType *arrayType = (SEType *)malloc(sizeof(SEType));
    arrayType->kind = ARRAY;
    arrayType->size = type->size * arraySize;
    arrayType->parent = arrayType;
    arrayType->array.size = arraySize;
    arrayType->array.kind = type->kind;
    arrayType->array.type = type;
    return SEParseVarDec(var->child, arrayType, assignable);
  } else {
    // register ID in local scope
    const char *name = var->child->sval;
    if (STSearchCurr(name) != NULL) {
      if (getCurrentStackType() == STACK_STRUCTURE) {
        throwErrorS(SE_STRUCT_FIELD_DUPLICATE, var->child->line, name);
      } else {
        throwErrorS(SE_VARIABLE_DUPLICATE, var->child->line, name);
      }
    } else if (STSearchStru(name) != NULL) {
      // variables cannot share name with structures.
      throwErrorS(SE_VARIABLE_DUPLICATE, var->child->line, name);
    } else {
      STInsertCurr(var->child->sval, type);
      CLog(FG_GREEN, "new variable \"%s\"", var->child->sval);
    }
    if (assignable) {
      // we don't care about the chain except the type
      DUMMY_FIELD_CHAIN.head->type = type;
      return DUMMY_FIELD_CHAIN;
    } else {
      SEFieldChain chain;
      SEField *field = (SEField *)malloc(sizeof(SEField));
      field->name = var->child->sval;
      field->kind = type->kind;
      field->type = type;
      field->next = NULL;
      chain.head = chain.tail = field;
      return chain; // chain of length 1
    }
  }
  Panic("should not reach here");
}

// Parse a variable list.
#define malloc(s) NO_MALLOC_ALLOWED_VAR_LIST(s)
SEFieldChain SEParseVarList(STNode *list) {
  AssertSTNode(list, "VarList");
  SEFieldChain chain = SEParseParamDec(list->child);
  if (list->child->next) {
    SEFieldChain tail = SEParseVarList(list->child->next->next);
    chain.tail->next = tail.head;
    chain.tail = tail.tail;
  }
  return chain;
}
#undef malloc

// Parse a parameter declaration.
#define malloc(s) NO_MALLOC_ALLOWED_PARAM_DEC(s)
SEFieldChain SEParseParamDec(STNode *pdec) {
  AssertSTNode(pdec, "ParamDec");
  SEType *type = SEParseSpecifier(pdec->child);
  return SEParseVarDec(pdec->child->next, type, false);
}
#undef malloc

// Parse arguments list. Return a field chain.
SEFieldChain SEParseArgs(STNode *args) {
  AssertSTNode(args, "Args");
  SEType *type = SEParseExp(args->child);
  SEField *field = (SEField *)malloc(sizeof(SEField));
  field->name = NULL;
  field->kind = type->kind;
  field->type = type;
  field->next = NULL;
  SEFieldChain chain = { field, field };
  if (args->child->next) {
    SEFieldChain tail = SEParseArgs(args->child->next->next);
    chain.tail->next = tail.head;
    chain.tail = tail.tail;
  }
  return chain;
}

/**
 * Helper functions: dump, compare and destroy.
 * We don't need malloc from here any more.
 * */
#define malloc(s) NO_MALLOC_ALLOWED_HELPERS(s)

// Print the info of the type.
void SEDumpType(const SEType *type) {
#ifdef DEBUG
  switch (type->kind) {
    case VOID:
      Log("VOID");
      break;
    case BASIC:
      Log("%s", type->basic == INT ? "INT" : "FLOAT");
      break;
    case ARRAY:
      Log("%d-array of", type->array.size);
      SEDumpType(type->array.type);
      break;
    case STRUCTURE:
      Log("STRUCTURE");
      break;
    case FUNCTION:
      Log("FUNCTION");
      break;
  }
#else
  return;
#endif
}

// Get parent type in disjoint-set-union.
// Path is compressed for better performance.
SEType *SEGetParentType(SEType *t) {
  if (t->parent == t) return t;
  return t->parent = SEGetParentType(t->parent);
}

// Compare two types, return true if they are same.
bool SECompareType(SEType *t1, SEType *t2) {
  Log("%p (%d) vs %p (%d)", t1, t1->kind, t2, t2->kind);
  t1 = SEGetParentType(t1);
  t2 = SEGetParentType(t2);
  if (t1->kind != t2->kind) return false;
  if (t1->parent == t2->parent) return true;
  SEField *f1 = NULL, *f2 = NULL;
  switch (t1->kind) {
    case VOID:
      return t2->kind == VOID;
    case BASIC:
      return t1->basic == t2->basic;
    case ARRAY:
      // if (t1->array.size != t2->array.size) return false;
      return SECompareType(t1->array.type, t2->array.type);
    case STRUCTURE:
      if (SECompareField(t1->structure, t2->structure)) {
        t2->parent = t1;
        return true;
      } else return false;
    case FUNCTION:
      if (SECompareType(t1->function.type, t2->function.type) &&
          SECompareField(t1->function.signature, t2->function.signature)) {
        t2->parent = t1;
        return true;
      } else return false;
    default:
      Panic("compare %p (kind %d) not implemented", t1, t1->kind);
  }
  Panic("should not reach here");
  return false;
}

// Compare two field chains, return true if they are same.
bool SECompareField(SEField *f1, SEField *f2) {
  while (f1 != NULL && f2 != NULL) {
    if (!SECompareType(f1->type, f2->type)) return false;
    f1 = f1->next;
    f2 = f2->next;
  }
  return f1 == NULL && f2 == NULL;
}

// Destroy the type in memory until INT or FLOAT is met
void SEDestroyType(SEType *type) {
  SEField *temp = NULL, *field = NULL;
  switch (type->kind) {
    case VOID:
    case BASIC:
      return;
    case ARRAY: {
      // structures must be destroyed individually
      if (type->array.kind != STRUCTURE) {
        SEDestroyType(type->array.type);
      }
      free(type);
      return;
    }
    case STRUCTURE: {
      SEDestroyField(type->structure);
      free(type);
      return;
    }
    case FUNCTION: {
      if (!type->function.defined) {
        // undefined function detected when destroying its type
        throwErrorS(SE_FUNCTION_DECLARED_NOT_DEFINED, type->function.node->line, type->function.node->child->sval);
      }
      if (type->function.type->kind != STRUCTURE) {
        // structures must be destroyed individually
        SEDestroyType(type->function.type);
      }
      if (type->function.signature != &STATIC_FIELD_VOID &&
          type->function.signature != &STATIC_FIELD_INT) {
        SEDestroyField(type->function.signature);
      }
      free(type);
      return;
    }
    default:
      Panic("destroy %p (kind %d) not implemented", type, type->kind);
  }
  Panic("should not reach here");
}

// Destroy a chained field.
void SEDestroyField(SEField *field) {
  SEField *next = NULL;
  if (field == &DUMMY_FIELD) return;
  if (field == &STATIC_FIELD_VOID) return;
  if (field == &STATIC_FIELD_INT) return;
  while (field != NULL) {
    next = field->next;
    if (field->kind != STRUCTURE) {
      // structures must be destroyed individually
      SEDestroyType(field->type);
    }
    free(field);
    field = next;
  }
}
