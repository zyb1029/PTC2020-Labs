#include "token.h"
#include "debug.h"
#include <stdlib.h>

enum ENUM_RELOP RELOP_REV(enum ENUM_RELOP relop) {
  switch (relop) {
  case RELOP_IV:
    return RELOP_IV;
  case RELOP_LT:
    return RELOP_GE;
  case RELOP_LE:
    return RELOP_GT;
  case RELOP_GE:
    return RELOP_LT;
  case RELOP_GT:
    return RELOP_LE;
  case RELOP_EQ:
    return RELOP_NE;
  case RELOP_NE:
    return RELOP_EQ;
  }
  Assert(0, "unknown relop type");
}

GETYYLVAL(unsigned int, i) {
  return (unsigned int)strtol(str, (char **)NULL, 0);
}

GETYYLVAL(float, f) { return strtof(str, (char **)NULL); }

GETYYLVAL(enum ENUM_RELOP, r) {
  if (len == 0)
    return RELOP_IV;
  switch (str[0]) {
  case '=':
    return RELOP_EQ;
  case '!':
    return RELOP_NE;
  case '<':
    return len == 1 ? RELOP_LT : RELOP_LE;
  case '>':
    return len == 1 ? RELOP_GT : RELOP_GE;
  default:
    return RELOP_IV;
  }
}

GETYYLVAL(void, s) { strcpy(yylval.sval, str); }
