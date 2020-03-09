#include <stdlib.h>
#include "token.h"

GETYYLVAL(int, i) {
  return (int)strtol(str, (char **)NULL, 0);
}

GETYYLVAL(float, f) {
  return strtof(str, (char **)NULL);
}

GETYYLVAL(enum ENUM_RELOP, r) {
  if (len == 0) return RELOP_IV;
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

GETYYLVAL(void, s) {
  strcpy(yylval.sval, str);
}

