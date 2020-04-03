#include "type.h"
#include "table.h"
#include "semantics.h"
#include "syntax.tab.h"

#define DEBUG
#include "debug.h"

extern bool hasErrorS;
extern STNode *stroot;

const STError SETable[] = {
  {  0, "Pseudo error", "" },
  {  1, "Use of undefined variable ", "" },
  {  2, "Use of undefined function ", "" },
  {  3, "Duplicated name ", " of variables" },
  {  4, "Redefinition of function ", "" },
  {  5, "Type mismatched for assignment", "" },
  {  6, "Cannot assign to rvalue", "" },
  {  7, "Type mismatched for operands", "" },
  {  8, "Type mismatched for return" },
  {  9, "Arguments mismatched for function ", "" },
  { 10, "Array access on non-array variable ", "" },
  { 11, "Function call on non-function variable ", "" },
  { 12, "Non integer offset of an array", "" },
  { 13, "Field access on non-struct variable", "" },
  { 14, "Access to undefined field ", " in struct" },
  { 15, "Redefinition or initialization of field ", " in struct" },
  { 16, "Duplicated name ", " of struct" },
  { 17, "Use of undefined struct ", "" },
  { 18, "Function ", " declared but not defined" },
  { 19, "Inconsistent declaration of function ", "" },
};

// Main entry of semantic scan
void semanticScan() {
  CLog(FG_YELLOW, "Before prepare");
  STPrepare();
  CLog(FG_YELLOW, "After prepare");
  //checkSemantics(stroot, stroot);
  SEParseExtDefList(stroot->child);
  CLog(FG_YELLOW, "Before destroy");
  STDestroy();
  CLog(FG_YELLOW, "After destroy");
}

// Parse and check semantics of the current node.
void checkSemantics(STNode *node, STNode *parent) {
  if (node->empty) return;
  if (!strcmp(node->name, "DefList")) {
    SEParseDefList(node, true);
  } else if (!strcmp(node->name, "Exp")) {
    SEType *type = SEParseExp(node);
  } else {
    for (STNode *child = node->child; child != NULL; child = child->next) {
      checkSemantics(child, node);
    }
  }
}

// Throw an semantic error.
void throwErrorS(enum SemanticErrors id, int line, const char *name) {
  hasErrorS = true;
  fprintf(stderr, "Error type %d at Line %d: ", id, line);
  if (name != NULL) {
    fprintf(stderr, "%s\"%s\"%s", SETable[id].message1, name, SETable[id].message2);
  } else {
    fprintf(stderr, "%s", SETable[id].message1);
  }
  fprintf(stderr, ".\n");
}
