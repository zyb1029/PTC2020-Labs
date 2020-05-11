#ifndef OPT_H
#define OPT_H

#include <stdbool.h>

struct IROperand;

// Optimize const values.
typedef struct OCNode {
  bool is_var;
  int number;
  int value;
  int timestamp;
  bool active;
} OCNode;

void optimize();

bool OCUpdateOperand(struct IROperand *op);

OCNode *OCFind(struct IROperand op);
void OCActivate(struct IROperand op);
void OCDeactivate(struct IROperand op);
void OCInsert(struct IROperand op, int value);
int OCComp(const void *a, const void *b);

#endif
