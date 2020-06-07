#ifndef ASM_H
#define ASM_H

#include "ir.h"

void assemble(FILE *file);

void ASTranslateList(FILE *file, IRCodeList list);
void ASTranslateCode(FILE *file, IRCode *code);

#endif
