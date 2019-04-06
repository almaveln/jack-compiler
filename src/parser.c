

#include <stdlib.h>
#include <zconf.h>
#include "parser.h"

static Class *parse_class();
static void parse_class_var_dec(Tokenizer *tokenizer, SymbolTable *gTable);
static Function *parse_subroutine(Tokenizer *tokenizer, Class *class);
static void parse_var_dec(Tokenizer *tokenizer, Function *func);
static void parse_param_list(Tokenizer *tokenizer, Function *func, char *className);
static void parse_subroutine_body(Tokenizer *tokenizer, Function *func);
static Vector *parse_statements(Tokenizer *tokenizer);
static ReturnStmt * parse_return(Tokenizer *tokenizer);
static DoStmt *parse_do(Tokenizer *tokenizer);
static WhileStmt *parse_while(Tokenizer *tokenizer);
static IfStmt *parse_if(Tokenizer *tokenizer);
static LetStmt *parse_let(Tokenizer *tokenizer);
static Expression *parse_expression(Tokenizer *tokenizer);
static ExpressionList *parse_expression_list(Tokenizer *tokenizer);
static SubroutineCall *parse_subroutine_call(Tokenizer *tokenizer);
static Term *parse_term(Tokenizer *tokenizer);

static void print_funcs(Vector *funcs);
static void print_expr(Expression *expr);
static void print_exprs(Vector *exprs);
static void print_subcall(SubroutineCall *subCall);
static void print_stmts(Vector *stmts);

Class *build_ast(Tokenizer *tokenizer) {
  Class *class = parse_class(tokenizer);

  // for debugging purposes
  // xprintf("class name: %s\n", class->name);
  // print_symbol_table(class->gTable);
  // print_funcs(class->functions);

  return class;
}

static Class *new_class(char *className) {
  Class *class = malloc(sizeof(Class));
  class->gTable = init_table();
  class->functions = new_vec();
  class->name = className;
  return class;
}

static Function *new_function(KeyWord funcKind, char *funcName, char *returnType) {
  Function *func = malloc(sizeof(Function));
  func->funcKind = funcKind;
  func->name = funcName;
  func->lTable = init_table();
  func->returnType = returnType;
  func->statements = NULL;
  return func;
}

static void print_term(Term *term) {
  switch (term->type) {
    case TERM_INT:
      xprintf("%s", term->integer);
      break;
    case TERM_STR:
      xprintf("%s", term->str);
      break;
    case TERM_KEYWORD:
      xprintf("%i", term->kConst);
      break;
    case TERM_VAR:
      xprintf("%s", term->varName);
      break;
    case TERM_EXPR_PARENS: {
      xprintf("(");
      print_expr(term->expr);
      xprintf(")");
      break;
    }
    case TERM_TERM_PAIR: {
      TermPair *pair = term->termPair;
      xprintf("%c", pair->op);
      print_term(pair->term);
      break;
    }
    case TERM_SUB_CALL: {
      print_subcall(term->subCall);
      break;
    }
    case TERM_ARRAY: {
      xprintf("%s[", term->array->varName);
      print_expr(term->array-> expr);
      xprintf("]");
      break;
    }
    default:
      break;
  }
}

static void print_subcall(SubroutineCall *subCall) {
  enum SubCallType type = subCall->type;
  if (type == PLAIN_SUB_CALL) {
    xprintf("%s(", subCall->subroutineName);
    if (subCall->exprList != NULL) {
      print_exprs(subCall->exprList->expressions);
    }
    xprintf(")");
  } else {
    xprintf("%s.%s(", subCall->target, subCall->subroutineName);
    if (subCall->exprList != NULL) {
      print_exprs(subCall->exprList->expressions);
    }
    xprintf(")");
  }
}

static void print_expr(Expression *expr) {
  print_term(expr->firstTerm);

  if (expr->termPairs == NULL) return;

  for (int i = 0; i < expr->termPairs->len; i++) {
    TermPair *pair = vec_get(expr->termPairs, i);
    xprintf(" %c ", pair->op);
    print_term(pair->term);
  }
}

static void print_exprs(Vector *exprs) {
  for (int i = 0; i < exprs->len; i++) {
    Expression *expr = vec_get(exprs, i);
    print_expr(expr);

    if (i != exprs->len - 1) {
      xprintf(", ");
    }
  }
}

static void print_stmt(Statement *stmt) {
  switch (stmt->type) {
    case RETURN_STMT: {
      xprintf("return ");
      Expression *expr = stmt->retStmt->expr;
      if (expr != NULL) {
        print_expr(stmt->retStmt->expr);
      }
      xprintf("\n");
      break;
    }
    case LET_STMT: {
      xprintf("let %s", stmt->letStmt->name);
      if (stmt->letStmt->type == LET_TYPE_ARRAY) {
        xprintf("[");
        print_expr(stmt->letStmt->firstExpr);
        xprintf("]");
      }
      xprintf(" = ");
      print_expr(stmt->letStmt->secondExpr);
      xprintf("\n");
      break;
    }
    case IF_STMT: {
      xprintf("if ( ");
      print_expr(stmt->ifStmt->expr);
      xprintf(") {\n");
      print_stmts(stmt->ifStmt->ifStmts);
      xprintf("}\n");
      if (stmt->ifStmt->elseStmts != NULL) {
        xprintf("else {\n");
        print_stmts(stmt->ifStmt->elseStmts);
        xprintf("}\n");
      }
      break;
    }
    case WHILE_STMT: {
      xprintf("while (");
      print_expr(stmt->whileStmt->expr);
      xprintf(") {\n");
      print_stmts(stmt->whileStmt->stmts);
      xprintf("}\n");
      break;
    }
    case DO_STMT: {
      xprintf("do ");
      print_subcall(stmt->doStmt->call);
      xprintf("\n");
      break;
    }
    default:
      break;
  }
}

static void print_stmts(Vector *stmts) {
  for (int i = 0; i < stmts->len; i++) {
    Statement *stmt = vec_get(stmts, i);
    print_stmt(stmt);
  }
}

static void print_funcs(Vector *funcs) {
  for (int i = 0; i < funcs->len; i++) {
    Function *func = vec_get(funcs, i);
    xprintf("--------------------------------\n");
    xprintf("func name: %s; func kind: %s; return type: %s\n", func->name, keyword_to_string(func->funcKind), func->returnType);
    print_symbol_table(func->lTable);
    print_stmts(func->statements);
  }
}

static Class *parse_class(Tokenizer *tokenizer) {
  // 'class' className '{' classVarDec* subroutineDec* '}'
  advance(tokenizer);
  expect_class(tokenizer);
  char *className = expect_identifier(tokenizer);
  expect_symbol(tokenizer, '{');
  Class *class = new_class(className);

  while (is_class_var_dec(tokenizer)) {
    parse_class_var_dec(tokenizer, class->gTable);
  }

  while (is_subroutine(tokenizer)) {
    vec_push(class->functions, parse_subroutine(tokenizer, class));
  }

  expect_symbol(tokenizer, '}');

  // check to see if there are tokens that have not been compiled
  if (tokenizer->hasMoreTokens) raise_error(tokenizer);
  return class;
}

static void parse_class_var_dec(Tokenizer *tokenizer, SymbolTable *gTable) {
  // ( 'static' | 'field' ) type varName ( ',' varName)* ';'
  KeyWord keyWord = expect_keyword_n(tokenizer, 2, STATIC, FIELD);
  Kind curKind = transformToKind(keyWord);
  char *type = expect_type(tokenizer);
  char *varName = expect_identifier(tokenizer);
  define(gTable, varName, type, curKind);

  while (is_this_symbol(tokenizer, ',')) {
    expect_symbol(tokenizer, ',');
    varName = expect_identifier(tokenizer);
    define(gTable, varName, type, curKind);
  }

  expect_symbol(tokenizer, ';');
}

static Function *parse_subroutine(Tokenizer *tokenizer, Class *class) {
  // ('constructor' | 'function' | 'method') ('void' | type) subroutineName '(' parameterList ')' subroutineBody
  KeyWord funcKind = expect_keyword_n(tokenizer, 3, CONSTRUCTOR, FUNCTION, METHOD);
  char *returnType;
  if (is_equal_to(get_keyword(tokenizer), VOID)) {
    returnType = keyword_to_string(expect_keyword_n(tokenizer, 1, VOID));
  } else {
    returnType = expect_type(tokenizer);
  }

  char *funcName = expect_identifier(tokenizer);
  Function *func = new_function(funcKind, funcName, returnType);

  expect_symbol(tokenizer, '(');
  parse_param_list(tokenizer, func, class->name);
  expect_symbol(tokenizer, ')');
  parse_subroutine_body(tokenizer, func);
  return func;
}

static void parse_var_dec(Tokenizer *tokenizer, Function *func) {
  // 'var' type varName ( ',' varName)* ';'
  expect_keyword_n(tokenizer, 1, VAR);
  char *varType = expect_type(tokenizer);
  char *varName = expect_identifier(tokenizer);
  define(func->lTable, varName, varType, KIND_VAR);

  while(is_this_symbol(tokenizer, ',')) {
    expect_symbol(tokenizer, ',');
    varName = expect_identifier(tokenizer);
    define(func->lTable, varName, varType, KIND_VAR);
  }

  expect_symbol(tokenizer, ';');
}

static void parse_param_list(Tokenizer *tokenizer, Function *func, char *className) {
  // ((type varName) ( ',' type varName)*)?
  if (is_one_of(func->funcKind, 1, METHOD)) {
    define(func->lTable, "this", className, KIND_ARG);
  }

  if (!is_type(tokenizer)) return;
  char *paramType = expect_type(tokenizer);
  char *paramName = expect_identifier(tokenizer);
  define(func->lTable, paramName, paramType, KIND_ARG);

  while(is_this_symbol(tokenizer, ',')) {
    expect_symbol(tokenizer, ',');
    paramType = expect_type(tokenizer);
    paramName = expect_identifier(tokenizer);
    define(func->lTable, paramName, paramType, KIND_ARG);
  }
}

static void parse_subroutine_body(Tokenizer *tokenizer, Function *func) {
  // subroutineBody ('{' varDec* statements '}')
  expect_symbol(tokenizer, '{');

  while(is_var_dec(tokenizer)) {
    parse_var_dec(tokenizer, func);
  }

  func->statements = parse_statements(tokenizer);
  expect_symbol(tokenizer, '}');
}

//*============================  Statements ============================ */

static Vector *parse_statements(Tokenizer *tokenizer) {
  Vector *statements = new_vec();
  Statement *stmt;

  KeyWord keyWord = get_keyword(tokenizer);
  while(is_one_of(keyWord, 5, LET, IF, WHILE, DO, RETURN)) {
    stmt = malloc(sizeof(Statement));
    switch (keyWord) {
      case LET: {
        stmt->type = LET_STMT;
        stmt->letStmt = parse_let(tokenizer);
        break;
      }
      case IF: {
        stmt->type = IF_STMT;
        stmt->ifStmt = parse_if(tokenizer);
        break;
      }
      case WHILE: {
        stmt->type = WHILE_STMT;
        stmt->whileStmt = parse_while(tokenizer);
        break;
      }
      case DO:
        stmt->type = DO_STMT;
        stmt->doStmt = parse_do(tokenizer);
        break;
      case RETURN:
        stmt->type = RETURN_STMT;
        stmt->retStmt = parse_return(tokenizer);
        break;
      default:
        raise_error(tokenizer);
        break;
    }

    vec_push(statements, stmt);
    keyWord = get_keyword(tokenizer);
  }

  return statements;
}

static LetStmt *parse_let(Tokenizer *tokenizer) {
  //  'let' varName ( '[' expression ']' )? '=' expression ';'
  expect_keyword_n(tokenizer, 1, LET);

  LetStmt *letStmt = malloc(sizeof(LetStmt));
  letStmt->name = expect_identifier(tokenizer);

  if (is_this_symbol(tokenizer, '[')) {
    expect_symbol(tokenizer, '[');
    letStmt->firstExpr = parse_expression(tokenizer);
    letStmt->type = LET_TYPE_ARRAY;
    expect_symbol(tokenizer, ']');
  } else {
    letStmt->firstExpr = NULL;
    letStmt->type = LET_TYPE_PLAIN;
  }

  expect_symbol(tokenizer, '=');
  letStmt->secondExpr = parse_expression(tokenizer);
  expect_symbol(tokenizer, ';');

  return letStmt;
}

static IfStmt *parse_if(Tokenizer *tokenizer) {
  //  'if' '(' expression ')' '{' statements '}' ( 'else' '{' statements '}' )?
  expect_keyword_n(tokenizer, 1, IF);

  IfStmt *ifStmt = malloc(sizeof(IfStmt));

  expect_symbol(tokenizer, '(');
  ifStmt->expr = parse_expression(tokenizer);
  expect_symbol(tokenizer, ')');

  expect_symbol(tokenizer, '{');
  ifStmt->ifStmts = parse_statements(tokenizer);
  expect_symbol(tokenizer, '}');

  ifStmt->elseStmts = NULL;
  if (is_equal_to(get_keyword(tokenizer), ELSE)) {
    expect_keyword_n(tokenizer, 1, ELSE);
    expect_symbol(tokenizer, '{');
    ifStmt->elseStmts = parse_statements(tokenizer);
    expect_symbol(tokenizer, '}');
  }

  return ifStmt;
}

static WhileStmt *parse_while(Tokenizer *tokenizer) {
  // 'while' '(' expression ')' '{' statements '}'
  expect_keyword_n(tokenizer, 1, WHILE);

  WhileStmt *whileStmt = malloc(sizeof(WhileStmt));

  expect_symbol(tokenizer, '(');
  whileStmt->expr = parse_expression(tokenizer);
  expect_symbol(tokenizer, ')');

  expect_symbol(tokenizer, '{');
  whileStmt->stmts = parse_statements(tokenizer);
  expect_symbol(tokenizer, '}');
  return whileStmt;
}

static DoStmt *parse_do(Tokenizer *tokenizer) {
  // 'do' subroutineCall ';'
  DoStmt *doStmt = malloc(sizeof(DoStmt));
  expect_keyword_n(tokenizer, 1, DO);
  doStmt->call = parse_subroutine_call(tokenizer);
  expect_symbol(tokenizer, ';');
  return doStmt;
}

static ReturnStmt *parse_return(Tokenizer *tokenizer) {
  // 'return' expression? ';'
  ReturnStmt *retStmt = malloc(sizeof(ReturnStmt));
  retStmt->expr = NULL;

  expect_keyword_n(tokenizer, 1, RETURN);
  if (is_term(tokenizer)) {
    retStmt->expr = parse_expression(tokenizer);
  }

  expect_symbol(tokenizer, ';');
  return retStmt;
}

//*============================  Expressions ============================ */

static Expression *parse_expression(Tokenizer *tokenizer) {
  // term (op term)*
  Expression *expr = malloc(sizeof(Expression));
  expr->firstTerm = parse_term(tokenizer);
  expr->termPairs = NULL;

  if (!is_op(tokenizer)) {
    return expr;
  }

  expr->termPairs = new_vec();

  while (is_op(tokenizer)) {
    TermPair *pair = malloc(sizeof(TermPair));

    pair->op = get_symbol(tokenizer);
    advance(tokenizer);

    pair->term = parse_term(tokenizer);
    vec_push(expr->termPairs, pair);
  }

  return expr;
}

static ExpressionList *parse_expression_list(Tokenizer *tokenizer) {
  // (expression ( ',' expression)* )?
  if (!is_term(tokenizer)) {
    return NULL;
  }

  ExpressionList *exprList = malloc(sizeof(ExpressionList));
  exprList->expressions = new_vec();
  vec_push(exprList->expressions, parse_expression(tokenizer));

  while (is_this_symbol(tokenizer, ',')) {
    expect_symbol(tokenizer, ',');
    vec_push(exprList->expressions, parse_expression(tokenizer));
  }

  return exprList;
}

static SubroutineCall *parse_subroutine_call(Tokenizer *tokenizer) {
  SubroutineCall *call = malloc(sizeof(SubroutineCall));

   char *name = expect_identifier(tokenizer);

  // subroutine call (name + expressionList)
  if (is_this_symbol(tokenizer, '(')) {
    call->type = PLAIN_SUB_CALL;
    call->subroutineName = name;
    expect_symbol(tokenizer, '(');
    call->exprList = parse_expression_list(tokenizer);
    expect_symbol(tokenizer, ')');
    return call;
  }

  // subroutine call (name + name + expressionList)
  if (is_this_symbol(tokenizer, '.')) {
    expect_symbol(tokenizer, '.');
    call->type = TARGET_SUB_CALL;
    call->target = name;
    call->subroutineName = expect_identifier(tokenizer);
    expect_symbol(tokenizer, '(');
    call->exprList = parse_expression_list(tokenizer);
    expect_symbol(tokenizer, ')');
    return call;
  }

  raise_error(tokenizer);
}

static Term *parse_term(Tokenizer *tokenizer) {
  // integerConstant | stringConstant | keywordConstant |
  // varName | varName '[' expression ']' | subroutineCall | '(' expression ')' | unaryOp term
  Term *term = malloc(sizeof(Term));

  if (is_int(tokenizer)) {
    term->type = TERM_INT;
    term->integer = get_int(tokenizer);
    advance(tokenizer);
    return term;
  }

  if (is_str(tokenizer)) {
    term->type = TERM_STR;
    term->str = get_string(tokenizer);
    advance(tokenizer);
    return term;
  }

  if (is_keyword_constant(tokenizer)) {
    term->type = TERM_KEYWORD;
    term->kConst = keyword_to_keywordConst(get_keyword(tokenizer));
    advance(tokenizer);
    return term;
  }

  if (is_this_symbol(tokenizer, '(')) {
    expect_symbol(tokenizer, '(');
    term->type = TERM_EXPR_PARENS;
    term->expr = parse_expression(tokenizer);
    expect_symbol(tokenizer, ')');
    return term;
  }

  if (is_unary_op(tokenizer)) {
    term->type = TERM_TERM_PAIR;
    term->termPair = malloc(sizeof(TermPair));
    term->termPair->op = get_symbol(tokenizer);
    advance(tokenizer);
    term->termPair->term = parse_term(tokenizer);
    return term;
  }

  if (!is_identifier(tokenizer)) {
    raise_error(tokenizer);
  }

  // means it is either subroutineCall or varName or varName + expressionList
  Token *token = lookahead(tokenizer);
  if (token->tokenType == SYMBOL) {
    // varName + expression
    if (token->symbol == '[') {
      char *name = expect_identifier(tokenizer);
      expect_symbol(tokenizer, '[');
      Expression *expr = parse_expression(tokenizer);
      expect_symbol(tokenizer, ']');

      Array *array = malloc(sizeof(Array));
      array->varName = name;
      array->expr = expr;

      term->type = TERM_ARRAY;
      term->array = array;
      return term;
    }

    // subroutineCall
    if (token->symbol == '(' || token->symbol == '.') {
      term->type = TERM_SUB_CALL;
      term->subCall = parse_subroutine_call(tokenizer);
      return term;
    }
  }

  // varName
  term->type = TERM_VAR;
  term->varName = expect_identifier(tokenizer);
  return term;
}

