#ifndef ASM_H
#define ASM_H

#include "ir.h"

extern const char *registers[];
#define _zero registers[0]
#define _at   registers[1]
#define _v0   registers[2]
#define _v1   registers[3]
#define _a0   registers[4]
#define _a1   registers[5]
#define _a2   registers[6]
#define _a3   registers[7]
#define _t0   registers[8]
#define _t1   registers[9]
// no more registers are used now

#define _MSB  0x80000000 // used to mark a positive offset
#define _MASK 0x7fffffff // get the absolute offset from $fp

void assemble(FILE *file);

void ASTranslateList(FILE *file, IRCodeList list);
void ASTranslateCode(FILE *file, IRCode *code);

void ASMoveRegister(FILE *file, const char *to, const char *from);
void ASLoadRegister(FILE *file, const char *reg, IROperand var);
void ASSaveRegister(FILE *file, const char *reg, IROperand var);

size_t ASPrepareFunction(IRCode *func, RBNode **root);
size_t ASRegisterVariable(IROperand *op, RBNode **root, size_t offset);

int ASComp(const void *a, const void *b);

#endif
