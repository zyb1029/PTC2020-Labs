#include <string.h>
#include "tree.h"
#include "type.h"
#include "table.h"
#include "semantics.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

#ifdef DEBUG
#define SEDEBUG
void SEDumpType(const SEType *type);
#else
#define SEDumpType(t) ;
#endif

SEType *STATIC_TYPE_INT   = NULL;
SEType *STATIC_TYPE_FLOAT = NULL;

void SEPrepare() {
  // int and float will never be a variable,
  // thus we can save them to the global ST.
  STATIC_TYPE_INT = (SEType *)malloc(sizeof(SEType));
  STATIC_TYPE_INT->kind = BASIC;
  STATIC_TYPE_INT->basic = INT;
  STInsertBase("int", STATIC_TYPE_INT);
  STATIC_TYPE_FLOAT = (SEType *)malloc(sizeof(SEType));
  STATIC_TYPE_FLOAT->kind = BASIC;
  STATIC_TYPE_FLOAT->basic = FLOAT;
  STInsertBase("float", STATIC_TYPE_INT);
}

SEType *SEParseExp(STNode *exp) {
  Assert(exp, "exp is null");
  Assert(!strcmp(exp->name, "Exp"), "not an exp");
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
        // function call
        Panic("Not implemented!");
      }
      break;
    case INT:
    case FLOAT: {
      return e1->token == INT ? STATIC_TYPE_INT : STATIC_TYPE_FLOAT;
    }
    default: { // Exp ??
      SEType *t1 = SEParseExp(e1);
      switch (e2->token) {
        case LB: {
          // Exp LB Exp RB
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
          // Exp DOT ID
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
              type = STATIC_TYPE_INT;
            }
            return type;
          }
        }
        case ASSIGNOP: {
          // Exp ASSIGNOP Exp
          SEType *t2 = SEParseExp(e3);
          if (SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e3); // same as gcc
          }
          CLog(FG_RED, "lvalue not checked!"); // FIXME
          return t1;
        }
        case AND:
        case OR: {
          // Exp AND/OR Exp
          SEType *t2 = SEParseExp(e3);
          if (SECompareType(STATIC_TYPE_INT, t1) ||
              SECompareType(STATIC_TYPE_INT, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2);
          }
          return STATIC_TYPE_INT; // always return INT
        }
        case RELOP: {
          // Exp RELOP Exp
          SEType *t2 = SEParseExp(e3);
          if (SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2);
          }
          return STATIC_TYPE_INT; // always return INT
        }
        default: {
          // Exp PLUS/MINUS/STAR/DIV Exp
          SEType *t2 = SEParseExp(e3);
          if (SECompareType(t1, t2)) {
            throwErrorS(SE_MISMATCHED_OPERANDS, e2);
          }
          return t1; // always treat as t1
        }
      }
    }
  }
  Panic("Should not reach here");
  return NULL;
}

SEType *SEParseSpecifier(STNode *specifier) {
  Assert(specifier, "specifier is null");
  Assert(specifier->next, "specifier at the end");
  Assert(!strcmp(specifier->name, "Specifier"), "not a specifier");
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
      const char *structID = tag->sval;
      {
        STPushStack();
        type->kind = STRUCTURE;
        type->structure = SEParseDefList(tag->next->next, false);
        STPopStack();
      }
      if (structID != NULL) {
        STInsertBase(structID, type); // struct has global scope
      }
      Log("Define struct %s", structID);
    } else {
      // STRUCT Tag
      STEntry *entry = STSearch(tag->sval);
      if (entry == NULL) {
        if (specifier->next->token == SEMI) {
          // only declare the struct
          Log("Declare struct %s", tag->sval);
          type->kind = STRUCTURE;
          type->structure = NULL;
          STInsertBase(tag->sval, type); // global scope
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

SEField *SEParseDefList(STNode *list, bool assignable) {
  if (list->empty) return NULL;
  SEField *tail = SEParseDefList(list->child->next, assignable);
  SEField *field = SEParseDef(list->child, tail, assignable);
  return field;
}

SEField *SEParseDef(STNode *def, SEField *tail, bool assignable) {
  SEType *type = SEParseSpecifier(def->child);
  SEField *field = SEParseDecList(def->child->next, type, tail, assignable);
  return field;
}

SEField *SEParseDecList(STNode *list, SEType *type, SEField *tail, bool assignable) {
  SEField *field = SEParseDec(list->child, type, assignable);
  if (list->child->next) {
    // Dec COMMA DecList
    field->next = SEParseDecList(list->child->next->next, type, tail, assignable);
  } else {
    // Dec -> tail
    field->next = tail;
  }
  return field;
}

SEField *SEParseDec(STNode *dec, SEType *type, bool assignable) {
  SEField *field = SEParseVarDec(dec->child, type);
  if (dec->child->next != NULL) {
    if (!assignable) {
      throwErrorS(SE_STRUCT_FIELD_INITIALIZED, dec->child->next);
    }
    SEType *expType = SEParseExp(dec->child->next->next);
    if (SECompareType(field->type, expType)) {
      throwErrorS(SE_MISMATCHED_ASSIGNMENT, dec->child->next);
    }
  }
  STInsertCurr(field->name, field->type); // register in local scope
  return field;
}

SEField *SEParseVarDec(STNode *var, SEType *type) {
  if (var->child->next) {
    // VarDec LB INT RB
    SEType *arrayType = (SEType *)malloc(sizeof(SEType));
    arrayType->kind = ARRAY;
    arrayType->array.size = var->child->next->next->ival;
    arrayType->array.elem = type;
    return SEParseVarDec(var->child, arrayType);
  } else {
    // ID
    SEField *field = (SEField *)malloc(sizeof(SEField));
    field->name = var->child->sval;
    field->type = type;
    field->next = NULL;
    CLog(FG_GREEN, "new variable \"%s\"", field->name);
    return field;
  }
}

#ifdef SEDEBUG
void SEDumpType(const SEType *type) {
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
}
#endif

bool SECompareType(const SEType *t1, const SEType *t2) {
  if (t1->kind != t2->kind) return false;
  SEField *f1 = NULL, *f2 = NULL;
  switch (t1->kind) {
    case BASIC:
      return t1->basic == t2->basic;
      break;
    case ARRAY:
      if (t1->array.size != t2->array.size) return false;
      return SECompareType(t1->array.elem, t2->array.elem);
      break;
    case STRUCTURE:
      f1 = t1->structure;
      f2 = t2->structure;
      while (f1 != NULL && f2 != NULL) {
        if (!SECompareType(f1->type, f2->type)) return false;
        f1 = f1->next;
        f2 = f2->next;
      }
      return f1 == NULL && f2 == NULL;
      break;
    case FUNCTION:
    default:
      Panic("not implemented");
  }
  return false;
}

/** DEPRECATED!!!
SEType *SECopyType(const SEType *type) {
  SEType *ret = (SEType *)malloc(sizeof(SEType));
  SEField *field = NULL, *head = NULL, *tail = NULL;
  switch (type->kind) {
    case BASIC:
      ret->basic = type->basic;
      break;
    case ARRAY:
      ret->array.size = type->array.size;
      ret->array.elem = SECopyType(type->array.elem);
      break;
    case STRUCTURE:
      field = type->structure;
      while (field != NULL) {
        tail = (SEField *)malloc(sizeof(SEField));
        tail->name = field->name;
        tail->type = SECopyType(field->type);
        tail->next = NULL;
        if (head == NULL) {
          head = tail;
          ret->structure = head;
        } else {
          head->next = tail;
          head = tail;
        }
        field = field->next;
      }
      break;
    case FUNCTION:
    default:
      Panic("not implemented");
  }
  return ret;
}
*/

void SEDestroyType(SEType *type) {
  SEField *temp = NULL, *field = NULL;
  switch (type->kind) {
    case BASIC:
      break;
    case ARRAY:
      SEDestroyType(type->array.elem);
      break;
    case STRUCTURE:
      field = type->structure;
      while (field != NULL) {
        temp = field;
        field = field->next;
        free(temp);
      }
      break;
    case FUNCTION:
    default:
      Panic("not implemented");
  }
  free(type);
  return;
}
