#include <string.h>
#include "type.h"
#include "table.h"
#include "syntax.tab.h"

STStack *baseStack = NULL;
STStack *currStack = NULL;

void STPushStack() {
  STStack *top = (STStack *)malloc(sizeof(STStack));
  top->root = NULL;
  top->prev = currStack;
  currStack = top;
}

void STPopStack() {
  STStack *temp = currStack;
  currStack = currStack->prev;
  RBDestroy(&(temp->root), STRBDestroy);
  free(temp);
}

void STInsert(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->type = SECopyType(type);
  RBInsert(&(currStack->root), (void *)entry, STRBCompare);
}

STEntry *STSearch(const char *id) {
  STEntry *target = (STEntry *)malloc(sizeof(STEntry));
  target->id = id;

  STStack *cur = currStack;
  STEntry *result = NULL;
  while (cur != NULL) {
    RBNode *candidate = RBSearch(&(cur->root), target, STRBCompare);
    if (candidate && !STRBCompare(candidate->value, target)) {
      result = (STEntry *)(candidate->value);
      break;
    }
    cur = cur->prev;
  }
  free(target);
  return result;
}

int STRBCompare(const void *p1, const void *p2) {
  return strcmp(((const STEntry *)p1)->id, ((const STEntry *)p2)->id);
}

void STRBDestroy(void *p) {
  SEDestroyType((SEType *)p);
}

