%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07 */
  #define YYDEBUG true // <- parser debugger switch
%}

%locations

%{
  #include <stdio.h>
  #include "relop.h"
  #include "lex.yy.c"
  void yyerror(char *);
%}

%union {
  int             ival;
  float           fval;
  enum ENUM_RELOP rval;
}

%token TYPE ID
%token <ival> INT
%token <fval> FLOAT
%token SEMI COMMA
%token LC RC
%token STRUCT RETURN IF WHILE

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left  OR
%left  AND
%left  <rval> RELOP
%left  PLUS MINUS
%left  STAR DIV
%right NOT
%left  DOT LB RB LP RP

%%
/* A.1.2 High-level Definitions */
Program: ExtDefList
  ;
ExtDefList: ExtDef ExtDefList
  | /* empty */
  ;
ExtDef: Specifier ExtDecList SEMI
  | Specifier SEMI
  | Specifier FunDec CompSt
  ;
ExtDecList: VarDec
  | VarDec COMMA ExtDecList
  ;

/* A.1.3 Specifiers */
Specifier: TYPE
  | StructSpecifier
  ;
StructSpecifier: STRUCT OptTag LC DefList RC
  | STRUCT Tag
  ;
OptTag: ID
  | /* EMPTY */
  ;
Tag: ID
  ;

/* A.1.4 Declarators */
VarDec: ID
  | VarDec LB INT RB
  ;
FunDec: ID LP VarList RP
  | ID LP RP
  ;
VarList: ParamDec COMMA VarList
  | ParamDec
  ;
ParamDec: Specifier VarDec
  ;

/* A.1.5 Statements */
CompSt: LC DefList StmtList RC
  ;
StmtList: Stmt StmtList
  | /* EMPTY */
  ;
Stmt: Exp SEMI
  | CompSt
  | RETURN Exp SEMI
  | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
  | IF LP Exp RP Stmt ELSE Stmt
  | WHILE LP Exp RP Stmt
  ;

/* A.1.6 Local Definitions */
DefList: Def DefList
  | /* EMPTY */
  ;
Def: Specifier DecList SEMI
  ;
DecList: Dec
  | Dec COMMA DecList
  ;
Dec: VarDec
  | VarDec ASSIGNOP Exp
  ;

/* A.1.7 Expressions */
Exp: Exp ASSIGNOP Exp
  | Exp AND Exp
  | Exp OR Exp
  | Exp RELOP Exp
  | Exp PLUS Exp
  | Exp MINUS Exp
  | Exp STAR Exp
  | Exp DIV Exp
  | LP Exp RP
  | MINUS Exp
  | NOT Exp
  | ID LP Args RP
  | ID LP RP
  | Exp LB Exp RB
  | Exp DOT ID
  | ID
  | INT { printf("%d\n", $1); }
  | FLOAT { printf("%f\n", $1); }
  ;
Args: Exp COMMA Args
  | Exp
  ;

%%
void yyerror(char *msg) {
  fprintf(stderr, "yyerror: %s\n", msg);
}
int _warp_yyparse() {
#if YYDEBUG
  yydebug = 1;
#endif
  return yyparse();
}