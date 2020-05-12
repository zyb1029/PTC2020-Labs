#ifndef OPT_H
#define OPT_H

#include <stdbool.h>

struct IROperand;

#define TEM 0
#define VAR 1
#define ADD 2
#define MEM 3

// Optimize const values.
typedef struct OCNode {
  bool is_var;
  int number;
  int value;
  int reserved; // kind of RHS
  int timestamp;
  bool important; // important jump_cond and its dependencies
  bool active;    // whether the value is used afterwards
} OCNode;

void optimize();

bool OCReplace(struct IROperand *op);
bool OCReplace2(struct IROperand *op);

void OCCreate(struct IROperand op);
void OCUpdate(struct IROperand op, int value);
void OCUpdate2(struct IROperand op, int value, int reserved);
OCNode *OCFind(struct IROperand op);
void OCInvalid(struct IROperand op);
void OCImportant(struct IROperand op);
void OCActivate(struct IROperand op);
void OCDeactivate(struct IROperand op);
int OCComp(const void *a, const void *b);

#endif
