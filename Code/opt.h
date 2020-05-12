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
  bool important; // important jump_cond and its dependencies
  bool active;    // whether the value is used afterwards
} OCNode;

void optimize();

bool OCReplace(struct IROperand *op);
bool OCReplace2(struct IROperand *op);

void OCCreate(struct IROperand op);
void OCUpdate(struct IROperand op, int value);
OCNode *OCFind(struct IROperand op);
void OCInvalid(struct IROperand op);
void OCImportant(struct IROperand op);
void OCActivate(struct IROperand op);
void OCDeactivate(struct IROperand op);
int OCComp(const void *a, const void *b);

#endif
