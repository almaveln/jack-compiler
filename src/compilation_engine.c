
#include <stdlib.h>
#include <zconf.h>
#include <ctype.h>
#include <string.h>
#include "compilation_engine.h"
#include "lexer.h"


CompilationEngine *new_engine(char *fileName, Class *class) {
  CompilationEngine *engine = malloc(sizeof(CompilationEngine));
  engine->writer = init_vmWriter(fileName);
  engine->ast = class;
  engine->curFunc = NULL;
  engine->labelCounter = 0;
  return engine;
}

/*============================ Compile routines ============================ */

static void compile_subroutine(CompilationEngine *engine, Function *func);
static void compile_expression(CompilationEngine *engine, Expression *expr);
static void compile_subroutineCall(CompilationEngine *engine, SubroutineCall *call);
static void compile_term(CompilationEngine *engine, Term *term);
static void compile_statement(CompilationEngine *engine, Statement *stmt);
static void compile_subroutineBody(CompilationEngine *engine, Vector *stmts);
static void compile_return(CompilationEngine *engine, ReturnStmt *stmt);
static void compile_do(CompilationEngine *engine, DoStmt *stmt);
static void compile_if(CompilationEngine *engine, IfStmt *stmt);
static void compile_let(CompilationEngine *engine, LetStmt *stmt);
static void compile_while(CompilationEngine *engine, WhileStmt *stmt);
static void compile_expression_list(CompilationEngine *engine, ExpressionList *list);
static void compile_operator(CompilationEngine *engine, char op);
static void compile_unary_operator(CompilationEngine *engine, char op);
static Segment kind_to_segment(Kind kind);
static int find_index(CompilationEngine *engine, char *identName);
static Kind find_kind(CompilationEngine *engine, char *identName);
static char *find_type(CompilationEngine *engine, char *identName);
static void alloc_mem(CompilationEngine *engine, int nwords);

void compile_file(CompilationEngine *engine) {
  Class *class = engine->ast;
  Vector *funcs = class->functions;

  for (int i = 0; i < funcs->len; i++) {
    Function *func = vec_get(funcs, i);
    compile_subroutine(engine, func);
  }
}

static void compile_subroutine(CompilationEngine *engine, Function *func) {
  write_func(engine->writer, engine->ast->name, func->name, varCount(func->lTable, KIND_VAR));
  Vector *stmts = func->statements;
  engine->curFunc = func;
  compile_subroutineBody(engine, stmts);
}

static void compile_subroutineBody(CompilationEngine *engine, Vector *stmts) {
  if (engine->curFunc->funcKind == CONSTRUCTOR) {
    alloc_mem(engine, varCount(engine->ast->gTable, KIND_FIELD));
    write_pop_i(engine->writer, SEGMENT_POINTER, 0);
  }

  if (engine->curFunc->funcKind == METHOD) {
    write_push_i(engine->writer, SEGMENT_ARG, 0);
    write_pop_i(engine->writer, SEGMENT_POINTER, 0);
  }

  for (int i = 0; i < stmts->len; i++) {
    Statement *stmt = vec_get(stmts, i);
    compile_statement(engine, stmt);
  }
}

static void compile_statement(CompilationEngine *engine, Statement *stmt) {
  switch (stmt->type) {
    case LET_STMT:
      compile_let(engine, stmt->letStmt);
      break;
    case IF_STMT:
      compile_if(engine, stmt->ifStmt);
      break;
    case WHILE_STMT:
      compile_while(engine, stmt->whileStmt);
      break;
    case RETURN_STMT:
      compile_return(engine, stmt->retStmt);
      break;
    case DO_STMT:
      compile_do(engine, stmt->doStmt);
      break;
    default:
      xprintf("%i, this statement is not implemented", stmt->type);
      exit(EXIT_FAILURE);
  }
}

char *new_label(char *label, int salt) {
  StringBuilder *sb = new_sb();
  sb_concat_strings(sb, 1, label);
  sb_append_i(sb, salt);
  return sb_get(sb);
}

static void compile_if(CompilationEngine *engine, IfStmt *stmt) {
  char *elseLabel = new_label("IF_FALSE", engine->labelCounter);
  char *endLabel = new_label("IF_END", engine->labelCounter);
  engine->labelCounter++;

  compile_expression(engine, stmt->expr);
  write_arithmetic(engine->writer, NOT);
  write_if(engine->writer, elseLabel);

  compile_subroutineBody(engine, stmt->ifStmts);
  write_goto(engine->writer, endLabel);

  write_label(engine->writer, elseLabel);
  if (stmt->elseStmts != NULL) {
    compile_subroutineBody(engine, stmt->elseStmts);
  }

  write_label(engine->writer, endLabel);
}

static void compile_do(CompilationEngine *engine, DoStmt *stmt) {
  compile_subroutineCall(engine, stmt->call);
  // remove a value from the stack in order to avoid stackoverflow
  write_pop_i(engine->writer, SEGMENT_TEMP, 0);
}

static void compile_let(CompilationEngine *engine, LetStmt *stmt) {
  switch(stmt->type) {
    case LET_TYPE_PLAIN: {
      compile_expression(engine, stmt->secondExpr);
      int index = find_index(engine, stmt->name);
      Kind kind = find_kind(engine, stmt->name);
      write_pop_i(engine->writer, kind_to_segment(kind), index);
      break;
    }
    case LET_TYPE_ARRAY: {
      int index = find_index(engine, stmt->name);
      Kind kind = find_kind(engine, stmt->name);

      if (kind == KIND_NONE) {
        xprintf("%s is not defined", stmt->name);
        exit(EXIT_FAILURE);
      }

      compile_expression(engine, stmt->secondExpr);

      compile_expression(engine, stmt->firstExpr);
      write_push_i(engine->writer, kind_to_segment(kind), index);
      write_arithmetic(engine->writer, ADD);

      write_pop_i(engine->writer, SEGMENT_POINTER, 1);
      write_pop_i(engine->writer, SEGMENT_THAT, 0);
      break;
    }
    default:
      xprintf("unknown let statement type");
      exit(EXIT_FAILURE);
  }
}

static void compile_while(CompilationEngine *engine, WhileStmt *stmt) {
  char *whileStart = new_label("WHILE_START", engine->labelCounter);
  char *whileFalse = new_label("WHILE_FALSE", engine->labelCounter);
  engine->labelCounter++;

  write_label(engine->writer, whileStart);

  compile_expression(engine, stmt->expr);
  write_arithmetic(engine->writer, NOT);
  write_if(engine->writer, whileFalse);

  compile_subroutineBody(engine, stmt->stmts);
  write_goto(engine->writer, whileStart);

  write_label(engine->writer, whileFalse);
}

static void compile_return(CompilationEngine *engine, ReturnStmt *stmt) {
  Expression *expr = stmt->expr;
  if (expr != NULL) {
    compile_expression(engine, stmt->expr);
  } else {
    write_push_i(engine->writer, SEGMENT_CONST, 0);
  }
  write_return(engine->writer);
}

static void compile_expression(CompilationEngine *engine, Expression *expr) {
  compile_term(engine, expr->firstTerm);

  if (expr->termPairs == NULL) return;

  for (int i = 0; i < expr->termPairs->len; i++) {
    TermPair *pair = vec_get(expr->termPairs, i);
    compile_term(engine, pair->term);
    compile_operator(engine, pair->op);
  }
}

static void compile_subroutineCall(CompilationEngine *engine, SubroutineCall *call) {
  enum SubCallType type = call->type;

  int nArgs = 0;

  // method call
  if (type == PLAIN_SUB_CALL) {
    // push "this" value onto the stack
    write_push_i(engine->writer, SEGMENT_POINTER, 0);
    if (call->exprList != NULL) {
      nArgs = call->exprList->expressions->len;
      compile_expression_list(engine, call->exprList);
    }

    write_call(engine->writer, engine->ast->name, call->subroutineName, nArgs + 1);
    return;
  }

  if (call->exprList != NULL) {
    nArgs = call->exprList->expressions->len;
  }

  char *targetType = find_type(engine, call->target);

  // static function call
  if (targetType == NULL) {
    if (call->exprList != NULL) {
      compile_expression_list(engine, call->exprList);
    }

    write_call(engine->writer, call->target, call->subroutineName, nArgs);
    return;
  }

  int index = find_index(engine, call->target);
  Kind kind = find_kind(engine, call->target);

  // method call on a object instance
  if (kind == KIND_FIELD || kind == KIND_VAR || kind == KIND_STATIC) {
    // field Ball ball; ball.getVal();
    // must be a field of another object; therefore, this should be set
    // we take what we need from (this + index)
    write_push_i(engine->writer, kind_to_segment(kind), index);
    nArgs = nArgs + 1;
  }

  if (call->exprList != NULL) {
    compile_expression_list(engine, call->exprList);
  }

  write_call(engine->writer, targetType, call->subroutineName, nArgs);
}

static void compile_term(CompilationEngine *engine, Term *term) {
  switch (term->type) {
    case TERM_INT:
      write_push(engine->writer, SEGMENT_CONST, term->integer);
      break;
    case TERM_STR: {
      size_t len = strlen(term->str);
      write_push_i(engine->writer, SEGMENT_CONST, len);
      write_call(engine->writer, "String", "new", 1);
      for (int i = 0; i < len; i++) {
        char chr = term->str[i];
        write_push_i(engine->writer, SEGMENT_CONST, chr);
        write_call(engine->writer, "String", "appendChar", 2);
      }
      break;
    }
    case TERM_KEYWORD: {
      switch (term->kConst) {
        case KC_TRUE:
          write_push_i(engine->writer, SEGMENT_CONST, 1);
          write_arithmetic(engine->writer, NEG);
          break;
        case KC_NULL: case KC_FALSE:
          write_push_i(engine->writer, SEGMENT_CONST, 0);
          break;
        case KC_THIS:
          // push the starting location of an object instance onto the stack
          // only used in constructor for "return this"
          write_push_i(engine->writer, SEGMENT_POINTER, 0);
          break;
        default:
          xprintf("undefined TERM_KEYWORD");
          exit(EXIT_FAILURE);
      }
      break;
    }
    case TERM_VAR: {
      int index = find_index(engine, term->varName);
      Kind kind = find_kind(engine, term->varName);
      write_push_i(engine->writer, kind_to_segment(kind), index);
      break;
    }
    case TERM_EXPR_PARENS: {
      compile_expression(engine, term->expr);
      break;
    }
    case TERM_TERM_PAIR: {
      TermPair *pair = term->termPair;
      compile_term(engine, pair->term);
      compile_unary_operator(engine, pair->op);
      break;
    }
    case TERM_SUB_CALL: {
      compile_subroutineCall(engine, term->subCall);
      break;
    }
    case TERM_ARRAY: {
      int index = find_index(engine, term->array->varName);
      Kind kind = find_kind(engine, term->array->varName);

      if (kind == KIND_NONE) {
        xprintf("%s is not defined", term->array->varName);
        exit(EXIT_FAILURE);
      }

      compile_expression(engine, term->array->expr);
      write_push_i(engine->writer, kind_to_segment(kind), index);
      write_arithmetic(engine->writer, ADD);
      write_pop_i(engine->writer, SEGMENT_POINTER, 1);
      write_push_i(engine->writer, SEGMENT_THAT, 0);

      break;
    }
    default:
      break;
  }
}

static void compile_expression_list(CompilationEngine *engine, ExpressionList *list) {
  Vector *exprs  = list->expressions;
  for (int i = 0; i < exprs->len; i++) {
    Expression *expr = vec_get(exprs, i);
    compile_expression(engine, expr);
  }
}

static void compile_operator(CompilationEngine *engine, char op) {
  // Multiplication  and  division  are  handled  using  the  OS  functions
  // Math.multiply() and Math.divide()
  switch(op) {
    case '+':
      write_arithmetic(engine->writer, ADD);
      break;
    case '-':
      write_arithmetic(engine->writer, SUB);
      break;
    case '*':
      write_call(engine->writer, "Math", "multiply", 2);
      break;
    case '/':
      write_call(engine->writer, "Math", "divide", 2);
      break;
    case '&':
      write_arithmetic(engine->writer, AND);
      break;
    case '|':
      write_arithmetic(engine->writer, OR);
      break;
    case '<':
      write_arithmetic(engine->writer, LT);
      break;
    case '>':
      write_arithmetic(engine->writer, GT);
      break;
    case '=':
      write_arithmetic(engine->writer, EQ);
      break;
    default:
      exit(EXIT_FAILURE);
  }
}

static void compile_unary_operator(CompilationEngine *engine, char op) {
  switch(op) {
    case '~':
      write_arithmetic(engine->writer, NOT);
      break;
    case '-':
      write_arithmetic(engine->writer, NEG);
      break;
    default:
      xprintf("%c is not implemented in compile_unary_operator", op);
      exit(EXIT_FAILURE);
  }
}

static Segment kind_to_segment(Kind kind) {
  switch (kind) {
    case KIND_VAR:
      return SEGMENT_LOCAL;
    case KIND_ARG:
      return SEGMENT_ARG;
    case KIND_FIELD:
      return SEGMENT_THIS;
    case KIND_STATIC:
      return SEGMENT_STATIC;
    default:
      xprintf("%i not implemented in kind_to_segment", kind);
      exit(EXIT_FAILURE);
  }
}

static int find_index(CompilationEngine *engine, char *identName) {
  int index = indexOf(engine->curFunc->lTable, identName);
  if (index == NO_IDENTIFIER) {
    index = indexOf(engine->ast->gTable, identName);

    if (index == NO_IDENTIFIER) {
      xprintf("%s is not defined in class %s", identName, engine->ast->name);
      exit(EXIT_FAILURE);
    }
  }

  return index;
}

static Kind find_kind(CompilationEngine *engine, char *identName) {
  Kind kind = kindOf(engine->curFunc->lTable, identName);
  if (kind == KIND_NONE) {
    kind = kindOf(engine->ast->gTable, identName);

    if (kind == KIND_NONE) {
      xprintf("%s is not defined in class %s", identName, engine->ast->name);
      exit(EXIT_FAILURE);
    }
  }

  return kind;
}

static char *find_type(CompilationEngine *engine, char *identName) {
  char *type = typeOf(engine->curFunc->lTable, identName);
  if (type == NULL) {
    type = typeOf(engine->ast->gTable, identName);
    return type;
  }

  return type;
}

static void alloc_mem(CompilationEngine *engine, int nwords) {
  // Memory.alloc(size), where size is the number of words
  write_push_i(engine->writer, SEGMENT_CONST, nwords);
  write_call(engine->writer, "Memory", "alloc", 1);
}