#include <string.h>
#include "type.h"
#include "table.h"
#include "semantics.h"
#include "syntax.tab.h"
#include "debug.h"

STStack *struStack = NULL;
STStack *funcStack = NULL;
STStack *currStack = NULL;

// Prepare the base (global) symbol table.
void STPrepare() {
  currStack = NULL;
  STPushStack(STACK_GLOBAL); // stack for structures
  struStack = currStack;
  STPushStack(STACK_GLOBAL); // stack for functions
  funcStack = currStack;
  STPushStack(STACK_LOCAL); // the global scope for variables
  SEPrepare();
}

// Destroy all symbol tables in system.
void STDestroy() {
  while (currStack != NULL) STPopStack();
}

// Get type of current ST stack.
enum STStackType getCurrentStackType() {
  return currStack->type;
}

// Create a new syntax table and push it into chain.
void STPushStack(enum STStackType type) {
  STStack *top = (STStack *)malloc(sizeof(STStack));
  Log("Push ST %p (type %d)", top, type);
  top->type = type;
  top->root = NULL;
  top->prev = currStack;
  currStack = top;
}

// Pop the last syntax table in chain and destroy it.
void STPopStack() {
  if (currStack == NULL) return;
  STStack *prev = currStack->prev;
  Log("Pop ST %p", currStack);
  RBDestroy(&(currStack->root), STRBDestroy);
  free(currStack);
  currStack = prev;
}

// Insert a symbol into stru (structure) ST.
void STInsertStru(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->number = -1;
  entry->type = type;
  Log("Insert to stru ST: %p %p \"%s\"", entry, type, id);
  RBInsert(&(struStack->root), (void *)entry, STRBCompare);
}

// Insert a symbol into func (function) ST.
void STInsertFunc(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->number = -1;
  entry->type = type;
  Log("Insert to func ST: %p %p \"%s\"", entry, type, id);
  RBInsert(&(funcStack->root), (void *)entry, STRBCompare);
}

// Insert a symbol into current (local) ST.
void STInsertCurr(const char *id, SEType *type) {
  STEntry *entry = (STEntry *)malloc(sizeof(STEntry));
  entry->id = id;
  entry->number = 0;
  entry->type = type;
  Log("Insert to curr ST: %p %p \"%s\"", entry, type, id);
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

// Search a symbol name in stru (structure) ST.
STEntry *STSearchStru(const char *id) {
  STEntry target;
  target.id = id;
  return STSearchAt(struStack, &target);
}

// Search a symbol name in func (function) ST.
STEntry *STSearchFunc(const char *id) {
  STEntry target;
  target.id = id;
  return STSearchAt(funcStack, &target);
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
  STEntry *entry = (STEntry *)p;
  if (currStack->type == STACK_GLOBAL ||
      (currStack->type == STACK_LOCAL && !entry->type->extended)) {
    // only destroy types in global ST or non-struct in local ST
    Log("Destroy from ST: %p %p \"%s\"", p, entry->type, entry->id);
    SEDestroyType(entry->type);
    if (entry->id[0] == ' ') {
      // anonymous object, destroy its name
      free((char *)entry->id);
    }
  }
  free(p);
}
