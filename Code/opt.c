#include "opt.h"
#include "ir.h"

extern IRCodeList irlist;

void optimize() { OPTJumpToNextLine(); }

void OPTJumpToNextLine() {
  for (IRCode *code = irlist.head, *next = NULL;
       code != NULL && code->next != NULL; code = next) {
    next = code->next;
    if (code->kind == IR_CODE_JUMP) {
      if (code->next->kind == IR_CODE_LABEL &&
          code->next->label.label.number == code->jump.dest.number) {
        irlist = IRRemoveCode(irlist, code);
      }
    }
  }
}
