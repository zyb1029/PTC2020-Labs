#include <string.h>
#include "type.h"
#include "table.h"
#include "syntax.tab.h"

STStack *baseStack = NULL;
STStack *currStack = NULL;

void STPrepare() {
  // prepare the base stack
  baseStack = (STStack *)malloc(sizeof(STStack));
  baseStack->root = NULL;
  baseStack->prev = NULL;
  currStack = baseStack;
  SEPrepare();
}

void STDestroy() {
  while (currStack != NULL) STPopStack();
}

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

void STInsertBase(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->type = type;
  RBInsert(&(baseStack->root), (void *)entry, STRBCompare);
}

void STInsertCurr(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->type = type;
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
