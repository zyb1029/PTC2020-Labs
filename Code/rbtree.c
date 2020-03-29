#include "rbtree.h"
#include <assert.h>

/**
 * This Red-Black Tree implementation is manually adapted from
 * https://www.geeksforgeeks.org/red-black-tree-set-3-delete-2/
 * which was implemented in C++. The adapated version is an
 * abstract RB tree, requiring user providing a cmp function.
 */

bool RBHasRedChild(RBNode *node) {
  if (node->left  && node->left->color  == RED) return true;
  if (node->right && node->right->color == RED) return true;
  return false;
}

bool RBIsLeftChild(RBNode *node) {
  if (!node->parent) assert(0);
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

RBNode *RBGetSuccessor(RBNode *node) {
  RBNode *cur = node;
  while (cur->left) {
    cur = cur->left;
  }
  return cur;
}

RBNode *RBGetReplacement(RBNode *node) {
  if (node->left && node->right) {
    // has 2 children
    return RBGetSuccessor(node->right);
  } else if (!node->left && !node->right) {
    // a leaf
    return NULL;
  } else if (node->left) {
    return node->left;
  } else {
    return node->right;
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

void RBFixRedRed(RBNode **root, RBNode *node) {
  // If node is root, color it black.
  if (node == *root) {
    node->color = BLACK;
    return;
  }

  RBNode *parent = node->parent;
  RBNode *grandParent = parent->parent;
  RBNode *uncle = RBGetUncle(node);
  if (parent->color != BLACK) {
    if (uncle && uncle->color == RED) {
      parent->color = BLACK;
      uncle->color = BLACK;
      grandParent->color = RED;
      RBFixRedRed(root, grandParent);
    } else {
      if (RBIsLeftChild(parent)) {
        if (RBIsLeftChild(node)) {
          RBSwapColors(parent, grandParent);
        } else {
          RBRotateLeft(root, parent);
          RBSwapColors(node, grandParent);
        }
        RBRotateRight(root, grandParent);
      } else {
        if (RBIsLeftChild(node)) {
          RBRotateRight(root, parent);
          RBSwapColors(node, grandParent);
        } else {
          RBSwapColors(parent, grandParent);
        }
        RBRotateLeft(root, grandParent);
      }
    }
  }
}

void RBFixBlackBlack(RBNode **root, RBNode *node) {
  if (node == *root) return;
  
  RBNode *sibling = RBGetSibling(node);
  RBNode *parent = node->parent;
  if (!sibling) {
    RBFixBlackBlack(root, parent);
  } else {
    if (sibling->color == RED) {
      parent->color = RED;
      sibling->color = BLACK;
      if (RBIsLeftChild(sibling)) {
        RBRotateRight(root, parent);
      } else {
        RBRotateLeft(root, parent);
      }
      RBFixBlackBlack(root, node);
    } else {
      if (RBHasRedChild(sibling)) {
        if (sibling->left && sibling->left->color == RED) {
          // left child red
          if (RBIsLeftChild(sibling)) {
            sibling->left->color = sibling->color;
            sibling->color = parent->color;
            RBRotateRight(root, parent);
          } else {
            sibling->left->color = parent->color;
            RBRotateRight(root, sibling);
            RBRotateLeft(root, parent);
          }
        } else {
          // right child red
          if (RBIsLeftChild(sibling)) {
            sibling->right->color = parent->color;
            RBRotateLeft(root, sibling);
            RBRotateRight(root, parent);
          } else {
            sibling->right->color = sibling->color;
            sibling->color = parent->color;
            RBRotateLeft(root, parent);
          }
        }
        parent->color = BLACK;
      } else {
        // 2 black children
        sibling->color = RED;
        if (parent->color == BLACK) {
          RBFixBlackBlack(root, parent);
        } else {
          parent->color = BLACK;
        }
      }
    }
  }
}

void RBDeleteNode(RBNode **root, RBNode *node) {
  RBNode *rep = RBGetReplacement(node);

  bool bothBlack = ((!rep || rep->color == BLACK) && node->color == BLACK);
  RBNode *parent = node->parent;
  
  if (rep == NULL) {
    // node is a leaf
    if (node == *root) {
      *root = NULL;
    } else {
      if (bothBlack) {
        RBFixBlackBlack(root, node);
      } else {
        RBNode *sibling = RBGetSibling(node);
        if (sibling) {
          sibling->color = RED;
        }
      }
      if (RBIsLeftChild(node)) {
        parent->left = NULL;
      } else {
        parent->right = NULL;
      }
    }
    free(node);
    return;
  }
  
  if (!node->left || !node->right) {
    // node has one child
    if (node == *root) {
      node->value = rep->value;
      node->left = node->right = NULL;
      free(rep);
    } else {
      if (RBIsLeftChild(node)) {
        parent->left = rep;
      } else {
        parent->right = rep;
      }
      free(node);
      rep->parent = parent;
      if (bothBlack) {
        RBFixBlackBlack(root, rep);
      } else {
        rep->color = BLACK;
      }
    }
    return;
  }
  RBSwapValues(node, rep);
  RBDeleteNode(root, rep);
}

void RBInsert(RBNode **root, void *value, int (*cmp)(const void *, const void *)) {
  if (!root) return;
  RBNode *node = (RBNode *)malloc(sizeof(RBNode));
  node->value = value;
  node->color = RED;
  node->left = node->right = node->parent = NULL;

  if (*root == NULL) {
    node->color = BLACK;
    *root = node;
  } else {
    RBNode *parent = RBSearch(root, value, cmp);
    int result = cmp(value, parent->value);
    if (result != 0) {
      node->parent = parent;
      if (result < 0) {
        parent->left = node;
      } else {
        parent->right = node;
      }
    }
    RBFixRedRed(root, node);
  }
}

RBNode *RBSearch(RBNode **root, void *value, int (*cmp)(const void *, const void *)) {
  if (!root) return NULL;
  RBNode *cur = *root;
  while (cur != NULL) {
    int result = cmp(value, cur->value);
    if (result == 0) {
      return cur;
    } else if (result < 0) {
      if (cur->left) {
        cur = cur->left;
      } else {
        break;
      }
    } else {
      if (cur->right) {
        cur = cur->right;
      } else {
        break;
      }
    }
  }
  return cur;
}

void RBDelete(RBNode **root, void *value, int (*cmp)(const void *, const void *)) {
  if (!root || !*root) return;
  RBNode *node = RBSearch(root, value, cmp);
  if (!node || cmp(value, node->value)) return;
  printf("node with value %d found\n", *(int*)value);
  RBDeleteNode(root, node);
}

void RBDestroy(RBNode **root) {
  if ((*root)->left)  RBDestroy(&(*root)->left);
  if ((*root)->right) RBDestroy(&(*root)->right);
  free(*root);
  root = NULL;
}
