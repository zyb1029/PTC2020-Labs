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

inline bool RBHasRedChild(RBNode *node) {
	if (node->left  && node->left->color  == RED) return true;
	if (node->right && node->right->color == RED) return true;
	return false;
}

inline bool RBIsLeftChild(RBNode *node) {
	if (!node->parent) return false;
  return node == node->parent->left;
}

inline RBNode *RBGetUncle(RBNode *node) {
  if (!node->parent || !node->parent->parent) return NULL;
  if (RBIsLeftChild(node->parent)) {
    return node->parent->parent->right;
  } else {
    return node->parent->parent->left;
  }
}

inline RBNode *RBGetSibling(RBNode *node) {
	if (!node->parent) return NULL;
  if (RBIsLeftChild(node)) {
    return node->parent->right;
  } else {
    return node->parent->left;
  }
}

inline void RBMoveNodeDown(RBNode *node, RBNode *newParent) {
  if (node->parent) {
		if (RBIsLeftChild(node)) {
			node->parent->left = newParent;
 		} else {
 			node->parent->right = newParent;
		}
  }
  newParent->parent = node->parent;
	node->parent = newParent;
}

#endif // RBTREE_H
