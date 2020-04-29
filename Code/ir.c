#include "debug.h"
#include "ir.h"

// Append a code to the end of list.
void IRAppendCode(struct IRCodeList *list, struct IRCode *code) {
  if (list->tail == NULL) {
    list->head = list->tail = code;
  } else {
    list->tail->next = code;
    code->prev = list->tail;
    list->tail = code;
  }
}

// Concat list2 to the end of list1.
void IRConcatLists(struct IRCodeList *list1, struct IRCodeList *list2) {
  if (list1->tail == NULL) {
    list1->head = list2->head;
    list1->tail = list2->tail;
  } else {
    list1->tail->next = list2->head;
    list2->head->prev = list1->tail;
    list1->tail = list2->tail;
  }
}

// Destory all codes in an IRCodeList.
void IRDestroyList(struct IRCodeList *list) {
  // Do not free the IRCodeList, it is static
  for (IRCode *code = list->head, *next = NULL; code != NULL; code = next) {
    next = code->next;  // safe loop
    free(code);
  }
}
