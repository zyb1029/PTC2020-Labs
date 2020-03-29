#ifndef RBTREE_H
#define RBTREE_H

#include <stdio.h>
#include <stdbool.h>

enum RBColor {
  RED,
  BLACK
};

typedef struct RBNode {
	void *val;
	enum RBColor color;
	struct RBNode *left, *right, *parent;
} RBNode;

bool RBHasRedChild(RBNode *node);
bool RBIsLeftChild(RBNode *node);
RBNode *RBGetUncle(RBNode *node);
RBNode *RBGetSibling(RBNode *node);
void RBMoveDown(RBNode *node, RBNode *newParent);

void RBRotateLeft(RBNode **root, RBNode *node);
void RBRotateRight(RBNode **root, RBNode *node);
void RBSwapColors(RBNode *n1, RBNode *n2);
void RBSwapValues(RBNode *n1, RBNode *n2);
void RBFixRedRed(RBNode **root, RBNode *node);
void RBFixBlackBlack(RBNode **root, RBNode *node);


#endif // RBTREE_H
