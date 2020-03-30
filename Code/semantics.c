#include "type.h"
#include "semantics.h"
#include "syntax.tab.h"

extern bool hasErrorS;
extern STNode *stroot;

const STError SETable[] = {
  {  0, false, "Pseudo error", "" },
  {  1,  true, "Use of undefined variable", "" },
  {  2,  true, "Use of undefined function", "" },
  {  3,  true, "Duplicated name", "of variable" },
  {  4,  true, "Redefinition of function", "" },
  {  5, false, "Type mismatched for assignment", "" },
  {  6, false, "Cannot assign to rvalue", "" },
  {  7, false, "Type mismatched for operands", "" },
  {  8, false, "Type mismatched for return" },
  {  9,  true, "Arguments mismatched for function", "" },
  { 10,  true, "Array access on non-array variable", "" },
  { 11,  true, "Function call on non-function variable", "" },
  { 12, false, "Non integer offset of an array", "" },
  { 13,  true, "Field access on non-struct variable", "" },
  { 14,  true, "Access to undefined field", "in struct" },
  { 15,  true, "Redefinition or initialization of field", "in struct" },
  { 16,  true, "Duplicated name", "of struct" },
  { 17,  true, "Use of undefined struct", "" },
  { 18,  true, "Function", "declared but not declared" },
  { 19,  true, "Inconsistent declaration of function", "" },
};

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

// main entry of semantic scan
void semanticScan() {
  // prepare the base stack
  baseStack = (STStack *)malloc(sizeof(STStack));
  baseStack->root = NULL;
  baseStack->prev = NULL;
  currStack = baseStack;
  checkSemantics(stroot, stroot);
}

void checkSemantics(STNode *node, STNode *parent) {
  if (node->empty) return;
  if (!strcmp(node->name, "Def")) {
    STNode *child = node->child;
    SEType *type = SECreateType(child);
  } else {
    for (STNode *child = node->child; child != NULL; child = child->next) {
      checkSemantics(child, node);
    }
  }
}

void throwErrorS(int id, STNode *node) {
  hasErrorS = true;
  fprintf(stderr, "Error type %d at Line %d: ", id, node->line);
  if (SETable[id].showId) {
    fprintf(stderr, "%s \"%s\" %s", SETable[id].message1, node->sval, SETable[id].message2);
  } else {
    fprintf(stderr, "%s", SETable[id].message1);
  }
  fprintf(stderr, ".\n");
}
