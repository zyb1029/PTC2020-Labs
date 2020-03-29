#include "semantics.h"
#include "syntax.tab.h"
#define SEMANTIC_DEBUG true // <- debug switch

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
  if (node->token == ID) {
    printf("%s %s:%s\n", parent->name, node->name, node->sval);
    printErrorS(0, node);
  }
  for (STNode *child = node->child; child != NULL; child = child->next) {
    checkSemantics(child, node);
  }
}

void printErrorS(int id, STNode *node) {
  hasErrorS = true;
  fprintf(stderr, "Error type %d at Line %d: ", id, node->line);
  if (SETable[id].showId) {
    fprintf(stderr, "%s \"%s\" %s\n", SETable[id].message1, node->sval, SETable[id].message2);
  } else {
    fprintf(stderr, "%s\n", SETable[id].message1);
  }
}
