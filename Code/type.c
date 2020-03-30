#include <string.h>
#include "tree.h"
#include "type.h"
#include "table.h"
#include "semantics.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

SEType *SEParseExp(STNode *exp) {
  Assert(exp, "exp is null");
  Assert(!strcmp(exp->name, "Exp"), "not an exp");
  
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
      type->kind = STRUCTURE;
      Panic("not implemented!"); /* FIXME */
      if (structID != NULL) {
        STInsert(structID, type);
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
          STInsert(tag->sval, type);
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
  Panic("Not implemented!");
}

SEField *SEParseDef(STNode *list, bool assignable) {
  Panic("Not implemented!");
}

SEField *SEParseDecList(STNode *list, bool assignable) {
  Panic("Not implemented!");
}

SEField *SEParseDec(STNode *dec, bool assignable) {
  Panic("Not implemented!");
}

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
