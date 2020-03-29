#include "rbtree.h"

bool RBHasRedChild(RBNode *node) {
	if (node->left  && node->left->color  == RED) return true;
	if (node->right && node->right->color == RED) return true;
	return false;
}

bool RBIsLeftChild(RBNode *node) {
	if (!node->parent) return false;
  return node == node->parent->left;
}

RBNode *RBGetUncle(RBNode *node) {
  if (!node->parent || !node->parent->parent) return NULL;
  if (RBIsLeftChild(node->parent)) {
    return node->parent->parent->right;
  } else {
    return node->parent->parent->left;
  }
}

RBNode *RBGetSibling(RBNode *node) {
	if (!node->parent) return NULL;
  if (RBIsLeftChild(node)) {
    return node->parent->right;
  } else {
    return node->parent->left;
  }
}

void RBSwapColors(RBNode *n1, RBNode *n2) {
  enum RBColor temp = n1->color;
	n1->color = n2->color;
	n2->color = temp;
}

void RBSwapValues(RBNode *n1, RBNode *n2) {
	void *temp = n1->value;
	n1->value = n2->value;
	n2->value = temp;
}

void RBMoveDown(RBNode *node, RBNode *newParent) {
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

void RBRotateLeft(RBNode **root, RBNode *node) {
  RBNode *newParent = node->right;
	if (node == *root) {
    *root = newParent;
  }
  RBMoveDown(node, newParent);
  if (newParent->left) {
    newParent->left->parent = node;
  }
  node->right = newParent->left;
  newParent->left = node;
}

void RBRotateRight(RBNode **root, RBNode *node) {
	RBNode *newParent = node->left;
	if (node == *root) {
		*root = newParent;
  }
  RBMoveDown(node, newParent);
	if (newParent->right) {
		newParent->right->parent = node;
  }
	node->left = newParent->right;
	newParent->right = node;
}
