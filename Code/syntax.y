%{
  /* Copyright, Tianyun Zhang @ Nanjing University. 2020-03-07 */
  #define YYDEBUG true // <- parser debugger switch
%}

%locations
%token-table

%{
  #include <stdio.h>
  #include "relop.h"
  #include "tree.h"

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

  /* Custom YYLTYPE, add a pointer to STNode */
  #define YYLTYPE YYLTYPE
  typedef struct YYLTYPE {
    int first_line, first_column;
    int last_line, last_column;
    STNode *st_node;
  } YYLTYPE;
  #define YYLTYPE_IS_DECLARED true
  #define YYLTYPE_IS_TRIVIAL  true

  /* Macro function to create STNodes for nterms */
  #define YYLLOC_DEFAULT(Cur, Rhs, N)                                                     \
    do {                                                                                  \
      if (N) {                                                                            \
        (Cur).first_line   = YYRHSLOC(Rhs, 1).first_line;                                 \
        (Cur).first_column = YYRHSLOC(Rhs, 1).first_column;                               \
        (Cur).last_line    = YYRHSLOC(Rhs, N).last_line;                                  \
        (Cur).last_column  = YYRHSLOC(Rhs, N).last_column;                                \
      } else {                                                                            \
        (Cur).first_line   = (Cur).last_line  = YYRHSLOC(Rhs, 0).last_line;               \
        (Cur).first_column = (Cur).last_column = YYRHSLOC(Rhs, 0).last_column;            \
      }                                                                                   \
      STNode *node = (STNode *)malloc(sizeof(STNode));                                    \
      node->line   = (Cur).first_line;                                                    \
      node->column = (Cur).first_column;                                                  \
      node->type   = yyr1[yyn];                                                           \
      node->name   = yytname[node->type];                                                 \
      if (N) {                                                                            \
        for (int st_child = 1; st_child < N - 1; ++st_child) {                            \
          if (YYRHSLOC(Rhs, st_child).st_node == NULL) {                                  \
            YYSTYPE *child_vsp = yyvsa + st_child; /* semantic value stack */             \
            STNode *child = (STNode *)malloc(sizeof(STNode));                             \
            child->line   = YYRHSLOC(Rhs, st_child).first_line;                           \
            child->column = YYRHSLOC(Rhs, st_child).first_column;                         \
            child->type   = child_vsp->type;                                              \
            child->name   = yytname[child_vsp->type];                                     \
            switch (child_vsp->type) {                                                    \
              case INT:                                                                   \
                child->ival = child_vsp->ival;                                            \
                break;                                                                    \
              case FLOAT:                                                                 \
                child->fval = child_vsp->fval;                                            \
                break;                                                                    \
              case RELOP:                                                                 \
                child->rval = child_vsp->rval;                                            \
                break;                                                                    \
              case ID:                                                                    \
                strcpy(child->sval, child_vsp->sval);                                     \
                break;                                                                    \
              default:                                                                    \
                break; /* value undefined */                                              \
            }                                                                             \
            YYRHSLOC(Rhs, st_child).st_node = child;                                      \
          }                                                                               \
          (YYRHSLOC(Rhs, st_child).st_node)->next = YYRHSLOC(Rhs, st_child + 1).st_node;  \
        }                                                                                 \
        node->child = YYRHSLOC(Rhs, 1).st_node, node->next = NULL;                        \
      } else {                                                                            \
        node->child = node->next = NULL;                                                  \
      }                                                                                   \
      (Cur).st_node = node;                                                               \
    } while (0)

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
%right NOT
%left  DOT LB RB LP RP

%printer { fprintf(stderr, "%d", yylval.ival); } INT
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
int yyparse_wrap() {
#if YYDEBUG
  yydebug = 1;
#endif
  return yyparse();
}
