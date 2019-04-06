

#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "lexer.h"
#include "symbol_table.h"

typedef struct Class {
  char *name;
  SymbolTable *gTable;
  Vector *functions;
} Class;

typedef struct Function {
  char *name;
  KeyWord funcKind;
  char *returnType;
  SymbolTable *lTable;
  Vector *statements;
} Function;

enum TermType {
  TERM_INT,
  TERM_STR,
  TERM_KEYWORD,
  TERM_VAR,
  TERM_ARRAY,
  TERM_SUB_CALL,
  TERM_EXPR_PARENS,
  TERM_TERM_PAIR
} TermType;

typedef struct Term {
  enum TermType type;
  union {
    char *integer;
    char *str;
    KeywordConst kConst;
    char *varName;
    struct Array *array;
    struct SubroutineCall *subCall;
    struct Expression *expr;
    struct TermPair *termPair;
  };
} Term;

typedef struct TermPair {
  char op;
  struct Term *term;
} TermPair;

typedef struct Expression {
  Term *firstTerm;
  Vector *termPairs;
} Expression;

typedef struct SubroutineCall {
  enum SubCallType {
    PLAIN_SUB_CALL,
    TARGET_SUB_CALL
  } type;
  char *target;
  char *subroutineName;
  struct ExpressionList *exprList;
} SubroutineCall;

typedef struct ExpressionList {
  Vector *expressions;
} ExpressionList;

typedef struct Array {
  char *varName;
  struct Expression *expr;
} Array;

typedef struct WhileStatement {
  Expression *expr;
  Vector *stmts;
} WhileStmt;

typedef struct ReturnStatement {
  Expression *expr;
} ReturnStmt;

typedef struct IfStatement {
  Expression *expr;
  Vector *ifStmts;
  Vector *elseStmts;
} IfStmt;

typedef struct LetStatement {
  enum LetType {
    LET_TYPE_PLAIN,
    LET_TYPE_ARRAY,
  } type;
  char *name;
  Expression *firstExpr;
  Expression *secondExpr;
} LetStmt;

typedef struct DoStatement {
  SubroutineCall *call;
} DoStmt;

typedef struct Statement {
  enum {
    RETURN_STMT,
    WHILE_STMT,
    IF_STMT,
    DO_STMT,
    LET_STMT
  } type;
  union {
    ReturnStmt *retStmt;
    WhileStmt *whileStmt;
    IfStmt *ifStmt;
    LetStmt *letStmt;
    DoStmt *doStmt;
  };
} Statement;


Class *build_ast(Tokenizer *tokenizer);

#endif //COMPILER_PARSER_H
