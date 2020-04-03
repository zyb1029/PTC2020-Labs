#include <string.h>
#include "tree.h"
#include "type.h"
#include "table.h"
#include "semantics.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

#ifdef DEBUG
#define AssertSTNode(node, str) \
  Assert(node, "node is null"); \
  Assert(!strcmp(node->name, str), "not a " str);
#else
#define AssertSTNode(node, str)
#endif

SEType _STATIC_TYPE_VOID, _STATIC_TYPE_INT, _STATIC_TYPE_FLOAT;
SEType *STATIC_TYPE_VOID, *STATIC_TYPE_INT, *STATIC_TYPE_FLOAT;
SEField STATIC_FIELD_VOID, DUMMY_FIELD;
const SEFieldChain DUMMY_FIELD_CHAIN = { &DUMMY_FIELD, &DUMMY_FIELD };

void SEPrepare() {
  STATIC_TYPE_VOID = &_STATIC_TYPE_VOID;
  STATIC_TYPE_VOID->kind = VOID;
  STATIC_FIELD_VOID.type = STATIC_TYPE_VOID;
  STATIC_FIELD_VOID.next = NULL;
  STATIC_TYPE_INT = &_STATIC_TYPE_INT;
  STATIC_TYPE_INT->kind = BASIC;
  STATIC_TYPE_INT->basic = INT;
  STATIC_TYPE_FLOAT = &_STATIC_TYPE_FLOAT;
  STATIC_TYPE_FLOAT->kind = BASIC;
  STATIC_TYPE_FLOAT->basic = FLOAT;
}

// Parse an expression. Only one type so we don't need a chain.
#define malloc(s) NO_MALLOC_ALLOWED_EXP(s)
SEType *SEParseExp(STNode *exp) {
  AssertSTNode(exp, "Exp");
  STNode *e1 = exp->child;
  STNode *e2 = e1 ? e1->next : NULL;
  STNode *e3 = e2 ? e2->next : NULL;
  switch (e1->token) {
    case LP:    // LP Exp RP
    case MINUS: // MINUS Exp
    case NOT:   // NOT Exp
      return SEParseExp(e2);
    case ID:
      if (e2 == NULL) {
        STEntry *entry = STSearch(e1->sval);
        if (entry == NULL) {
          // undefined variable, treat as int
          throwErrorS(SE_VARIABLE_UNDEFINED, e1);
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
          throwErrorS(SE_FUNCTION_UNDEFINED, e1);
          return STATIC_TYPE_INT;
        } else if (entry->type->kind != FUNCTION) {
          // call to a non-function variable
          throwErrorS(SE_ACCESS_TO_NON_FUNCTION, e1); // same as gcc
          return STATIC_TYPE_INT;
        }
        signature = e3->next ? SEParseArgs(e3).head : &STATIC_FIELD_VOID;
        if (!SECompareField(entry->type->function.signature, signature)) {
          throwErrorS(SE_MISMATCHED_SIGNATURE, e1);
        }
        return entry->type->function.type;
      }
      break;
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
            throwErrorS(SE_ACCESS_TO_NON_ARRAY, e2);
            return t1;
          }
          SEType *t2 = SEParseExp(e3);
          if (t2->kind != BASIC || t2->basic != INT) {
            throwErrorS(SE_NON_INTEGER_INDEX, e3);
          }
          return t1->array.elem;
        }
        case DOT: {
          CLog(FG_CYAN, "Exp DOT ID");
          if (t1->kind != STRUCTURE) {
            throwErrorS(SE_ACCESS_TO_NON_STRUCT, e2);
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
              throwErrorS(SE_STRUCT_FIELD_UNDEFINED, e3);
              type = STATIC_TYPE_INT; // treat as INT
            }
            return type;
          }
        }
        case ASSIGNOP: {
          bool lvalue = false;
          SEType *t2 = SEParseExp(e3);
          CLog(FG_CYAN, "Exp ASSIGNOP Exp");
          Log("DUMP LEFT:"), SEDumpType(t1);
          Log("DUMP RIGHT:"), SEDumpType(t2);
          if (!SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_ASSIGNMENT, e3); // same as gcc
          }
          if (e1->child->token == ID) lvalue = true;
          if (!lvalue && e1->child->next) {
            lvalue = e1->child->next->token == LB
                  || e1->child->next->token == DOT;
          }
          if (!lvalue) {
            // Not any of ID / Exp LB Exp RB / Exp DOT ID
            throwErrorS(SE_RVALUE_ASSIGNMENT, e1);
          }
          return t1;
        }
        case AND:
        case OR: {
          SEType *t2 = SEParseExp(e3);
          CLog(FG_CYAN, "Exp AND/OR Exp");
          Log("DUMP LEFT:"), SEDumpType(t1);
          Log("DUMP RIGHT:"), SEDumpType(t2);
          if (!SECompareType(STATIC_TYPE_INT, t1) ||
              !SECompareType(STATIC_TYPE_INT, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2);
          }
          return STATIC_TYPE_INT; // always return INT
        }
        case RELOP: {
          CLog(FG_CYAN, "Exp RELOP Exp");
          SEType *t2 = SEParseExp(e3);
          if (!SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2);
          }
          return STATIC_TYPE_INT; // always return INT
        }
        default: {
          CLog(FG_CYAN, "Exp PLUS/MINUS/STAR/DIV Exp");
          SEType *t2 = SEParseExp(e3);
          if (!SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2);
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
  Assert(specifier->next, "specifier at the end");
  STNode *child = specifier->child;
  SEType *type = (SEType *)malloc(sizeof(SEType));
  if (child->token == TYPE) {
    type->kind = BASIC;
    if (!strcmp(child->sval, "int")) {
      Log("INT");
      type->basic = INT;
    } else {
      Log("FLOAT");
      type->basic = FLOAT;
    }
  } else {
    Log("STRUCT");
    child = child->child; // STRUCT
    Assert(child->token == STRUCT, "child is not struct");
    STNode *tag = child->next;
    if (tag->next) {
      // define a new struct
      // STRUCT OptTag LC DefList RC
      {
        STPushStack();
        type->kind = STRUCTURE;
        type->structure = SEParseDefList(tag->next->next, false).head;
        STPopStack();
      }
      if (!tag->child->empty) {
        const char *name = tag->child->sval;
        CLog(FG_GREEN, "new structure \"%s\"", name);
        if (STSearchBase(name) != NULL) {
          throwErrorS(SE_STRUCT_DUPLICATE, tag->child);
        } else {
          STInsertBase(name, type); // struct has global scope
        }
      }
    } else {
      // STRUCT Tag
      const char *name = tag->child->sval;
      STEntry *entry = STSearch(name);
      if (entry == NULL) {
        if (specifier->next->token == SEMI) {
          // only declare the struct
          CLog(FG_GREEN, "dec structure %s", name);
          type->kind = STRUCTURE;
          type->structure = NULL;
          STInsertBase(name, type); // global scope
        } else {
          // undefined struct, treat as INT
          throwErrorS(SE_STRUCT_UNDEFINED, tag->child);
          type->kind = BASIC;
          type->basic = INT;
        }
      } else if (entry->type->kind != STRUCTURE) {
        // duplicated name of struct, treat as INT
        throwErrorS(SE_STRUCT_DUPLICATE, tag->child);
        type->kind = BASIC;
        type->basic = INT;
      } else {
        // FIXME: what if we declated a struct twice??
        if (entry->type->structure == NULL) {
          // declared but not defined, treat as INT
          throwErrorS(SE_STRUCT_UNDEFINED, tag->child);
          type->kind = BASIC;
          type->basic = INT;
        } else {
          type = entry->type;
        }
      }
    }
  }
  return type; 
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
  STEntry *entry = STSearch(name);
  SEType *func = NULL;
  SEField *signature = NULL;

  if (entry == NULL) CLog(FG_GREEN, "new function \"%s\"", name);

  STPushStack(); // treat signature as inner scope
  if (vars->next) {
    signature = SEParseVarList(vars).head;
  } else {
    signature = &STATIC_FIELD_VOID;
  }

  if (entry == NULL) {
    func = (SEType *)malloc(sizeof(SEType));
    func->kind = FUNCTION;
    func->function.node = fdec;
    func->function.defined = fdec->next->token != SEMI;
    func->function.type = type;
    func->function.signature = signature;
    STInsertBase(name, func); // global scope
  } else {
    func = entry->type;
    if (func->kind != FUNCTION) {
      throwErrorS(SE_FUNCTION_DUPLICATE, id); // treat as redefine
      func->kind = FUNCTION; // overriding original type to avoid UB
    }
    if (fdec->next->token != SEMI) {
      if (func->function.defined) {
        throwErrorS(SE_FUNCTION_DUPLICATE, id);
      } else {
        CLog(FG_GREEN, "def function \"%s\"", name);
        func->function.defined = true;
      }
    }
    if (!SECompareType(func->function.type, type)) {
      throwErrorS(SE_FUNCTION_CONFLICTING, id);
    }
    if (!SECompareField(func->function.signature, signature)) {
      throwErrorS(SE_FUNCTION_CONFLICTING, id);
    }
  }
  if (fdec->next->token != SEMI) {
    SEParseCompSt(fdec->next, type);
  }
  STPopStack();
}

// Parse a composed statement list and check for RETURN statements.
#define malloc(s) NO_MALLOC_ALLOWED_COMPST(s)
void SEParseCompSt(STNode *comp, SEType *type) {
  AssertSTNode(comp, "CompSt");
  // We do not need to push/pop stack, that should be done by the CALLER.
  // CompSt -> LC DefList StmtList RC
  SEParseDefList(comp->child->next, true);
  SEParseStmtList(comp->child->next->next, type);
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
    STPushStack();
    SEParseCompSt(stmt->child, type);
    STPopStack();
    return;
  } else {
    switch (stmt->child->token) {
      case RETURN: { // RETURN Exp SEMI
        SEType *ret = SEParseExp(stmt->child->next);
        if (!SECompareType(type, ret)) {
          throwErrorS(SE_MISMATCHED_RETURN, stmt->child);
        }
        return;
      }
      case IF: { // IF LP Exp RP Stmt [ELSE Stmt]
        STNode *enode = stmt->child->next->next;
        STNode *snode = enode->next->next;
        SEType *etype = SEParseExp(enode);
        if (!SECompareType(etype, STATIC_TYPE_INT)) {
          throwErrorS(SE_MISMATCHED_OPERANDS, enode);
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
          throwErrorS(SE_MISMATCHED_OPERANDS, enode);
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
      throwErrorS(SE_STRUCT_FIELD_INITIALIZED, dec->child->next);
    }
    SEType *expType = SEParseExp(dec->child->next->next);
    if (!SECompareType(chain.head->type, expType)) {
      throwErrorS(SE_MISMATCHED_ASSIGNMENT, dec->child->next);
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
    SEType *arrayType = (SEType *)malloc(sizeof(SEType));
    arrayType->kind = ARRAY;
    arrayType->array.size = var->child->next->next->ival;
    arrayType->array.elem = type;
    return SEParseVarDec(var->child, arrayType, assignable);
  } else {
    // register ID in local scope
    const char *name = var->child->sval;
    if (STSearch(name) != NULL) {
      throwErrorS(SE_VARIABLE_DUPLICATE, var->child);
    } else{
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
    SEFieldChain tail = SEParseParamDec(list->child->next->next);
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
    case BASIC:
      Log("%s", type->basic == INT ? "INT" : "FLOAT");
      break;
    case ARRAY:
      Log("%d-array of", type->array.size);
      SEDumpType(type->array.elem);
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

// Compare two types, return true if they are same.
bool SECompareType(const SEType *t1, const SEType *t2) {
  if (t1->kind != t2->kind) return false;
  SEField *f1 = NULL, *f2 = NULL;
  switch (t1->kind) {
    case VOID:
      return t2->kind == VOID;
    case BASIC:
      return t1->basic == t2->basic;
    case ARRAY:
      if (t1->array.size != t2->array.size) return false;
      return SECompareType(t1->array.elem, t2->array.elem);
    case STRUCTURE:
      return SECompareField(t1->structure, t2->structure);
    case FUNCTION:
      return SECompareType(t1->function.type, t2->function.type)
          && SECompareField(t1->function.signature, t2->function.signature);
    default:
      Panic("compare %d not implemented", t1->kind);
  }
  Panic("should not reach here");
  return false;
}

// Compare two field chains, return true if they are same.
bool SECompareField(const SEField *f1, const SEField *f2) {
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
      SEDestroyType(type->array.elem);
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
        throwErrorS(SE_FUNCTION_DECLARED_NOT_DEFINED, type->function.node->child);
      }
      SEDestroyType(type->function.type);
      if (type->function.signature != &STATIC_FIELD_VOID) {
        SEDestroyField(type->function.signature);
      }
      free(type);
      return;
    }
    default:
      Panic("destroy %d not implemented", type->kind);
  }
  Panic("should not reach here");
}

// Destroy a chained field.
void SEDestroyField(SEField *field) {
  SEField *temp = NULL;
  while (field != NULL) {
    temp = field;
    field = field->next;
    SEDestroyType(temp->type);
    free(temp);
  }
}
