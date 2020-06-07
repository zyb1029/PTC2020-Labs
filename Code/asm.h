#ifndef ASM_H
#define ASM_H

#include "ir.h"

void assemble(FILE *file);

void ASTranslateList(FILE *file, IRCodeList list);
void ASTranslateCode(FILE *file, IRCode *code);

size_t ASPrepareFunction(IRCode *func, RBNode **root);
size_t ASRegisterVariable(IROperand *op, RBNode **root, size_t offset);

int ASComp(const void *a, const void *b);

#endif
