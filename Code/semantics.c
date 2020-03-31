#include "type.h"
#include "table.h"
#include "semantics.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

extern bool hasErrorS;
extern STNode *stroot;

const STError SETable[] = {
  {  0, false, "Pseudo error", "" },
  {  1,  true, "Use of undefined variable ", "" },
  {  2,  true, "Use of undefined function ", "" },
  {  3,  true, "Duplicated name ", " of variables" },
  {  4,  true, "Redefinition of function ", "" },
  {  5, false, "Type mismatched for assignment", "" },
  {  6, false, "Cannot assign to rvalue", "" },
  {  7, false, "Type mismatched for operands", "" },
  {  8, false, "Type mismatched for return" },
  {  9,  true, "Arguments mismatched for function ", "" },
  { 10, false, "Array access on non-array variable", "" },
  { 11, false, "Function call on non-function variable", "" },
  { 12, false, "Non integer offset of an array", "" },
  { 13, false, "Field access on non-struct variable", "" },
  { 14, false, "Access to undefined field in struct", "" },
  { 15,  true, "Redefinition or initialization of field ", " in struct" },
  { 16,  true, "Duplicated name ", " of struct" },
  { 17,  true, "Use of undefined struct ", "" },
  { 18,  true, "Function ", " declared but not declared" },
  { 19,  true, "Inconsistent declaration of function ", "" },
};

// main entry of semantic scan
void semanticScan() {
  CLog(FG_YELLOW, "Before prepare");
  STPrepare();
  CLog(FG_YELLOW, "After prepare");
  checkSemantics(stroot, stroot);
  CLog(FG_YELLOW, "Before destroy");
  STDestroy();
  CLog(FG_YELLOW, "After destroy");
}

void checkSemantics(STNode *node, STNode *parent) {
  if (node->empty) return;
  if (!strcmp(node->name, "DefList")) {
    SEField *field = SEParseDefList(node, true);
  } else if (!strcmp(node->name, "Exp")) {
    SEType *type = SEParseExp(node);
  } else {
    for (STNode *child = node->child; child != NULL; child = child->next) {
      checkSemantics(child, node);
    }
  }
}

void throwErrorS(enum SemanticErrors id, STNode *node) {
  Assert(!SETable[id].showID || node, "node is NULL for error %d", id);
  hasErrorS = true;
  fprintf(stderr, "Error type %d at Line %d: ", id, node->line);
  if (SETable[id].showID) {
    fprintf(stderr, "%s\"%s\"%s", SETable[id].message1, node->sval, SETable[id].message2);
  } else {
    fprintf(stderr, "%s", SETable[id].message1);
  }
  fprintf(stderr, ".\n");
}
