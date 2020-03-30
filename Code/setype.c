#include "debug.h"
#include "sytree.h"
#include "setype.h"

SEType *SECreateType(STNode *node, STNode *parent) {
  Assert(node && parent, "node or parent is null");
}

bool SECompareType(const SEType *t1, const SEType *t2) {
  // TODO
  Panic("Please implement me");
  return false;
}

void SEDestroyType(SEType *type) {
  // TODO
  Panic("Please implement me");
  return;
}
