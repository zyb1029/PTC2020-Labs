%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07 */
  #define YYDEBUG true // <- parser debugger switch
%}

%{
  #include <stdio.h>
  #include "lex.yy.c"
  void yyerror(char *);
%}

%token TYPE INT FLOAT ID
%token ASSIGNOP RELOP
%token PLUS MINUS STAR DIV
%token AND OR DOT NOT
%token SEMI COMMA
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

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
  | IF LP Exp RP Stmt
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
  | INT
  | FLOAT
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