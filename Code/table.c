#include <string.h>
#include "type.h"
#include "table.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

bool destroyAllTypes = false;
STStack *baseStack = NULL;
STStack *currStack = NULL;

// Prepare the base (global) symbol table.
void STPrepare() {
  baseStack = (STStack *)malloc(sizeof(STStack));
  baseStack->root = NULL;
  baseStack->prev = NULL;
  currStack = baseStack;
  Log("Base ST at %p", baseStack);
  SEPrepare();
}

// Destroy all symbol tables in system.
void STDestroy() {
  Assert(destroyAllTypes == false, "STDestroy called more than once!");
  while (currStack != baseStack) STPopStack();
  destroyAllTypes = true;
  STPopStack(); // destroy all types in global scope
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

// Insert a symbol into base (global) ST.
void STInsertBase(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->type = type;
  Log("Insert to base ST: %p %p", entry, type);
  RBInsert(&(baseStack->root), (void *)entry, STRBCompare);
}

// Insert a symbol into current (local) ST.
void STInsertCurr(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->type = type;
  Log("Insert to curr ST: %p %p", entry, type);
  RBInsert(&(currStack->root), (void *)entry, STRBCompare);
}

// Search a symbol name in all STs.
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

// Search a symbol name in base (global) ST.
STEntry *STSearchBase(const char *id) {
  STEntry target;
  target.id = id;
  return STSearchAt(baseStack, &target);
}

// Search a symbol name in current (local) ST.
STEntry *STSearchCurr(const char *id) {
  STEntry target;
  target.id = id;
  return STSearchAt(currStack, &target);
}

// Search a symbol ENTRY in a specified ST **inner function**.
STEntry *STSearchAt(STStack *stack, STEntry *target) {
  RBNode *candidate = RBSearch(&(stack->root), target, STRBCompare);
  if (candidate && !STRBCompare(candidate->value, target)) {
    return (STEntry *)(candidate->value);
  } else {
    return NULL;
  }
}

// Compare two ST entries.
int STRBCompare(const void *p1, const void *p2) {
  return strcmp(((const STEntry *)p1)->id, ((const STEntry *)p2)->id);
}

// Destroy an ST entry.
void STRBDestroy(void *p) {
  Log("Destroy from ST: %p %p", p, ((STEntry *)p)->type);
  SEDestroyType(((STEntry *)p)->type, destroyAllTypes);
  free(p);
}
