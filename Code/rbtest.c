#include "rbtree.h"
#include <stdio.h>

int cmp(const void *p1, const void *p2) {
  const int *i1 = (const int *)p1;
  const int *i2 = (const int *)p2;
  return *i1 - *i2;
}

void inorder(RBNode *cur) {
  if (cur->left) inorder(cur->left);
  if (cur->right) inorder(cur->right);
  printf("%d ", *((int *)cur->value));
}

int val[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

RBNode *root = NULL;

int main() {
  for (int i = 0; i < 10; ++i) {
    printf("inserting %d\n", i + 1);
    RBInsert(&root, &val[i], cmp);
    inorder(root), printf("\n");
  }
  for (int i = 0; i < 10; ++i) {
    printf("searching %d\n", i + 1);
    RBSearch(&root, &val[i], cmp);
  }
  RBDelete(&root, &val[3], cmp);
  inorder(root), printf("\n");
  for (int i = 0; i < 10; ++i) {
    printf("deleting %d\n", i + 1);
    RBDelete(&root, (void *)&val[i], cmp);
    inorder(root), printf("\n");
  }
}
