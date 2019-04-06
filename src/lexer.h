

#ifndef COMPILER_TOKENIZER_H
#define COMPILER_TOKENIZER_H

#include <stdio.h>
#include <stdbool.h>
#include "util.h"

typedef enum {
  KEYWORD,
  SYMBOL,
  IDENTIFIER,
  INT_CONST,
  STRING_CONST
} TokenType;

typedef enum {
  CLASS,
  METHOD,
  FUNCTION,
  CONSTRUCTOR,
  INT,
  BOOLEAN,
  CHAR,
  VOID,
  VAR,
  STATIC,
  FIELD,
  LET,
  DO,
  IF,
  ELSE,
  WHILE,
  RETURN,
  TRUE,
  FALSE,
  cNULL,
  THIS
} KeyWord;

typedef enum KeywordConst {
  KC_TRUE,
  KC_FALSE,
  KC_NULL,
  KC_THIS
} KeywordConst;

typedef struct {
  FILE *inStream;
  bool hasMoreTokens;
  Vector *tokens;
  int current;
  int end;
  int lineNumber;
} Tokenizer;

typedef struct {
  TokenType tokenType;
  KeyWord keyword;
  char *intValue;
  char *identifier;
  char symbol;
  char *stringValue;
} Token;


Tokenizer *new_tokenizer(char *path);
TokenType advance(Tokenizer *tokenizer);
Token *lookahead(Tokenizer *tokenizer);

TokenType get_token_type(Tokenizer *tokenizer);
KeyWord get_keyword(Tokenizer *tokenizer);
char get_symbol(Tokenizer *tokenizer);
char *get_identifier(Tokenizer *tokenizer);
char *get_int(Tokenizer *tokenizer);
char *get_string(Tokenizer *tokenizer);
char *keyword_to_string(KeyWord keyWord);
KeywordConst keyword_to_keywordConst(KeyWord keyWord);

bool is_equal_to(int actual, int expected);
bool is_one_of(int actual, int nOfElements, ...);

bool is_int(Tokenizer *tokenizer);
bool is_str(Tokenizer *tokenizer);
bool is_identifier(Tokenizer *tokenizer);
bool is_class(Tokenizer *tokenizer);
bool is_class_var_dec(Tokenizer *tokenizer);
bool is_this_symbol(Tokenizer *tokenizer, char expected);
bool is_subroutine(Tokenizer *tokenizer);
bool is_type(Tokenizer *tokenizer);
bool is_var_dec(Tokenizer *tokenizer);
bool is_keyword_constant(Tokenizer *tokenizer);
bool is_op(Tokenizer *tokenizer);
bool is_unary_op(Tokenizer *tokenizer);
bool is_term(Tokenizer *tokenizer);

void raise_error(Tokenizer *tokenizer);
char *expect_identifier(Tokenizer *tokenizer);
char *expect_class(Tokenizer *tokenizer);
char expect_symbol(Tokenizer *tokenizer, char symbol);
KeyWord expect_keyword_n(Tokenizer *tokenizer, int n, ...);
char *expect_type(Tokenizer *tokenizer);

#endif //COMPILER_TOKENIZER_H
