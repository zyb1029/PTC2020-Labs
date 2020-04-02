#include <string.h>
#include "type.h"
#include "table.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

STStack *baseStack = NULL;
STStack *currStack = NULL;

void STPrepare() {
  // prepare the base stack
  baseStack = (STStack *)malloc(sizeof(STStack));
  baseStack->root = NULL;
  baseStack->prev = NULL;
  currStack = baseStack;
  Log("Base ST at %p", baseStack);
  SEPrepare();
}

void STDestroy() {
  while (currStack != NULL) STPopStack();
}

// Create a new syntax table and push it into chain.
void STPushStack() {
  STStack *top = (STStack *)malloc(sizeof(STStack));
  Log("Push ST %p", top);
  top->root = NULL;
  top->prev = currStack;
  currStack = top;
}

// Pop the last syntax table in chain and destroy it.
void STPopStack() {
  if (currStack == NULL) return;
  STStack *temp = currStack;
  Log("Pop ST %p", temp);
  currStack = currStack->prev;
  RBDestroy(&(temp->root), STRBDestroy);
  free(temp);
}

void STInsertBase(const char *id, SEType *type) {
  Assert(type->kind != DUMMY, "insert DUMMY type");
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->type = type;
  Log("Insert to base ST: %p %p", entry, type);
  RBInsert(&(baseStack->root), (void *)entry, STRBCompare);
}

void STInsertCurr(const char *id, SEType *type) {
  Assert(type->kind != DUMMY, "insert DUMMY type");
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->type = type;
  Log("Insert to curr ST: %p %p", entry, type);
  RBInsert(&(currStack->root), (void *)entry, STRBCompare);
}

STEntry *STSearch(const char *id) {
  STEntry target;
  target.id = id;

  STStack *cur = currStack;
  STEntry *result = NULL;
  while (cur != NULL) {
    result = STSearchAt(cur, &target);
    if (result != NULL) break;
    cur = cur->prev;
  }
  return result;
}

STEntry *STSearchBase(const char *id) {
  STEntry target;
  target.id = id;
  return STSearchAt(baseStack, &target);
}

STEntry *STSearchCurr(const char *id) {
  STEntry target;
  target.id = id;
  return STSearchAt(currStack, &target);
}

STEntry *STSearchAt(STStack *stack, STEntry *target) {
  RBNode *candidate = RBSearch(&(stack->root), target, STRBCompare);
  if (candidate && !STRBCompare(candidate->value, target)) {
    return (STEntry *)(candidate->value);
  } else {
    return NULL;
  }
}

int STRBCompare(const void *p1, const void *p2) {
  return strcmp(((const STEntry *)p1)->id, ((const STEntry *)p2)->id);
}

void STRBDestroy(void *p) {
  Log("Destroy from ST: %p %p", p, ((STEntry *)p)->type);
  SEDestroyType(((STEntry *)p)->type);
  free(p);
}
