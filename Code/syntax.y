%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07 */
  #define YYDEBUG false // <- parser debugger switch
%}

%locations
%token-table

%{
  #include <stdio.h>
  #include <stdbool.h>
  #include "token.h"
  #include "tree.h"

  /* Macro function to create STNodes for nterms */
  #define YYLLOC_DEFAULT(Cur, Rhs, N)                                                     \
    do {                                                                                  \
      if (N) {                                                                            \
        (Cur).first_line   = YYRHSLOC(Rhs, 1).first_line;                                 \
        (Cur).first_column = YYRHSLOC(Rhs, 1).first_column;                               \
        (Cur).last_line    = YYRHSLOC(Rhs, N).last_line;                                  \
        (Cur).last_column  = YYRHSLOC(Rhs, N).last_column;                                \
      } else {                                                                            \
        (Cur).first_line   = (Cur).last_line  = yylineno;                                 \
        (Cur).first_column = (Cur).last_column = yycolumn;                                \
      }                                                                                   \
      STNode *node = (STNode *)malloc(sizeof(STNode));                                    \
      node->line   = (Cur).first_line;                                                    \
      node->column = (Cur).first_column;                                                  \
      node->token  = -1; /* nterm is not a token */                                       \
      node->symbol = yyr1[yyn];                                                           \
      node->name   = yytname[node->symbol];                                               \
      node->empty  = N == 0;                                                              \
      if (N) {                                                                            \
        for (int child = 1; child <= N; ++child) {                                        \
          if ((YYRHSLOC(Rhs, child).st_node)->symbol == -1) {                             \
            YYSTYPE *cvsp = yyvsp - N + child;    /* access to semantic value stack */    \
            int symbol = YYTRANSLATE(cvsp->type); /* translate from token to symbol */    \
            (YYRHSLOC(Rhs, child).st_node)->symbol = symbol;                              \
            (YYRHSLOC(Rhs, child).st_node)->name   = yytname[symbol];                     \
            (YYRHSLOC(Rhs, child).st_node)->child  = NULL;                                \
            (YYRHSLOC(Rhs, child).st_node)->next   = NULL;                                \
          }                                                                               \
        }                                                                                 \
        for (int child = 1; child <= N - 1; ++child) { /* Link all but the last child */  \
          (YYRHSLOC(Rhs, child).st_node)->next = YYRHSLOC(Rhs, child + 1).st_node;        \
        }                                                                                 \
        node->child = YYRHSLOC(Rhs, 1).st_node, node->next = NULL;                        \
      } else {                                                                            \
        node->child = node->next = NULL;                                                  \
      }                                                                                   \
      (Cur).st_node = node;                                                               \
    } while (0)

  /* Custom error template */
  #define YY_(Msg) Msg

  #include "lex.yy.c"
  void yyerror(char *);
%}

%token TYPE ID
%token INT
%token FLOAT
%token SEMI COMMA
%token LC RC
%token STRUCT RETURN IF WHILE

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left  OR
%left  AND
%left  RELOP
%left  PLUS MINUS
%left  STAR DIV
%right NOT NEG
%left  DOT LB RB LP RP

%printer { fprintf(stderr, "%u", yylval.ival); } INT
%printer { fprintf(stderr, "%f", yylval.fval); } FLOAT
%printer { fprintf(stderr, "%s", yytext); }      RELOP

%%
/* A.1.2 High-level Definitions */
Program: ExtDefList { stroot = @$.st_node; }
  ;
ExtDefList: ExtDef ExtDefList
  | /* empty */
  ;
ExtDef: Specifier ExtDecList SEMI
  | Specifier SEMI
  | Specifier FunDec CompSt
  | error SEMI
  | error FunDec CompSt
  ;
ExtDecList: VarDec
  | VarDec COMMA ExtDecList
  ;

/* A.1.3 Specifiers */
Specifier: TYPE
  | StructSpecifier
  ;
StructSpecifier: STRUCT OptTag LC DefList RC
  | STRUCT OptTag LC error RC
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
  | error RP /* either ID or VarList */
  ;
VarList: ParamDec COMMA VarList
  | ParamDec
  ;
ParamDec: Specifier VarDec
  ;

/* A.1.5 Statements */
CompSt: LC DefList StmtList RC
  | LC DefList error RC
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
  | error SEMI
  | IF LP error RP Stmt %prec LOWER_THAN_ELSE
  | IF LP error RP Stmt ELSE Stmt
  | IF LP Exp RP error ELSE Stmt
  | WHILE LP error RP Stmt
  ;

/* A.1.6 Local Definitions */
DefList: Def DefList
  | /* EMPTY */
  ;
Def: Specifier DecList SEMI
  | Specifier error SEMI
  ;
DecList: Dec
  | Dec COMMA DecList
  ;
Dec: VarDec
  | VarDec ASSIGNOP Exp
  | error ASSIGNOP Exp
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
  | LP error RP
  | LP error SEMI /* worst case */
  | MINUS Exp %prec NEG
  | NOT Exp
  | ID LP Args RP
  | ID LP RP
  | ID LP error RP
  | ID LP error SEMI /* worst case */
  | Exp LB Exp RB
  | Exp LB error RB
  | Exp LB error SEMI /* worst case */
  | Exp DOT ID
  | ID
  | INT
  | FLOAT
  ;
Args: Exp COMMA Args
  | Exp
  ;

%%
extern int  errLineno;
extern bool hasErrorB;
void yyerror(char *msg) {
  hasErrorB = true;
  if (errLineno == yylineno) return; // one error per line
  else errLineno = yylineno;
  fprintf(stderr, "Error type B at Line %d: %s.\n", yylineno, msg);
}
int yyparse_wrap() {
#if YYDEBUG
  yydebug = 1;
#endif
  return yyparse();
}
