/**
 * The token types and helper macros/functions.
 * Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07
 * */

#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>
#include <string.h>

struct STNode; /* avoid cyclic include */

/* enumeration type for relational operators */
enum ENUM_RELOP {
  RELOP_IV, // invalid
  RELOP_LT, // <
  RELOP_LE, // <=
  RELOP_GT, // >
  RELOP_GE, // >=
  RELOP_EQ, // ==
  RELOP_NE, // !=
};

/* Custom YYSTYPE, use struct instead of union */
#define YYSTYPE YYSTYPE
typedef struct YYSTYPE {
  int type;
  union {
    int             ival;
    float           fval;
    enum ENUM_RELOP rval;
    char            sval[64];
  };
} YYSTYPE;
#define YYSTYPE_IS_DECLARED true
#define YYSTYPE_IS_TRIVIAL  true
extern YYSTYPE yylval;

/* Custom YYLTYPE, add a pointer to STNode */
#define YYLTYPE YYLTYPE
typedef struct YYLTYPE {
  int first_line, first_column;
  int last_line, last_column;
  struct STNode *st_node;
} YYLTYPE;
#define YYLTYPE_IS_DECLARED true
#define YYLTYPE_IS_TRIVIAL  true
extern YYLTYPE yylloc;

/* Inline functions for setting yylval */
#define GETYYLVAL(SPEC, TYPE) SPEC get##TYPE##Token(char* str, size_t len)
#define SETYYLVAL(TYPE) yylval.TYPE##val = get##TYPE##Token(yytext, (size_t)yyleng)
#define ACTYYLVAL(TYPE) get##TYPE##Token(yytext, (size_t)yyleng)

GETYYLVAL(int, i);
GETYYLVAL(float, f);
GETYYLVAL(enum ENUM_RELOP, r);
GETYYLVAL(void, s);

#endif // TOKEN_H
