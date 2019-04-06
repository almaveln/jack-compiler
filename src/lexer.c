
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <ctype.h>
#include <string.h>
#include <zconf.h>
#include "lexer.h"

static Map *keywords_table;

static Map *new_keyword_map();
static void add_next_token(Tokenizer *tokenizer);

Tokenizer *new_tokenizer(char *path) {
  FILE *inStream = fopen(path, "r");

  if (inStream == NULL) {
    exit(EXIT_FAILURE);
  }

  Tokenizer *tokenizer = malloc(sizeof(Tokenizer));
  tokenizer->inStream = inStream;
  tokenizer->hasMoreTokens = true;
  tokenizer->tokens = new_vec();
  tokenizer->current = -1;
  tokenizer->end = -1;
  tokenizer->lineNumber = 1;

  keywords_table = new_keyword_map();
  return tokenizer;
}

// ------------------------------- New methods --------------------------
Token *peek(Tokenizer *tokenizer) {
  return vec_get(tokenizer->tokens, tokenizer->end);
}

TokenType get_token_type(Tokenizer *tokenizer) {
  return ((Token *) vec_get(tokenizer->tokens, tokenizer->current))->tokenType;
}

KeyWord get_keyword(Tokenizer *tokenizer) {
  return ((Token *) vec_get(tokenizer->tokens, tokenizer->current))->keyword;
}

char get_symbol(Tokenizer *tokenizer) {
  return ((Token *) vec_get(tokenizer->tokens, tokenizer->current))->symbol;
}

char *get_identifier(Tokenizer *tokenizer) {
  return ((Token *) vec_get(tokenizer->tokens, tokenizer->current))->identifier;
}

char *get_int(Tokenizer *tokenizer) {
  return ((Token *) vec_get(tokenizer->tokens, tokenizer->current))->intValue;
}

char *get_string(Tokenizer *tokenizer) {
  return ((Token *) vec_get(tokenizer->tokens, tokenizer->current))->stringValue;
}

Token *lookahead(Tokenizer *tokenizer) {
  add_next_token(tokenizer);
  tokenizer->current--;
  return peek(tokenizer);
}

void close_tokenizer(Tokenizer *tokenizer) {
  if (tokenizer->inStream != NULL) {
    fclose(tokenizer->inStream);
    tokenizer->inStream = NULL;
  }

  free(tokenizer);
}

static Token *new_token(TokenType tokenType) {
  Token *token = malloc(sizeof(Token));
  token->tokenType = tokenType;
  return token;
}

static void add_token(Tokenizer *tokenizer, Token *token) {
  vec_push(tokenizer->tokens, token);
  tokenizer->current++;
  tokenizer->end++;
}

static bool had_to_catch_up_with_last_pos(Tokenizer *tokenizer) {
  if (tokenizer->current < tokenizer->end) {
    tokenizer->current = tokenizer->end;
    return true;
  }
  return false;
}

TokenType advance(Tokenizer *tokenizer) {
  add_next_token(tokenizer);
  return get_token_type(tokenizer);
}

static void add_next_token(Tokenizer *tokenizer) {
  if (!tokenizer->hasMoreTokens) {
    return;
  }

  if (had_to_catch_up_with_last_pos(tokenizer)) return;

  while (true) {
    int chr = getc(tokenizer->inStream);

    if (chr == EOF) {
      tokenizer->hasMoreTokens = false;
      break;
    }

    if (chr == '\n') {
      tokenizer->lineNumber++;
      continue;
    }

    // remove comments if they are present
    if (chr == '/') {
      int chrSecond = getc(tokenizer->inStream);

      if (chrSecond == '/') {
        while (getc(tokenizer->inStream) != '\n');
        tokenizer->lineNumber++;
        continue;
      } else if (chrSecond == '*') {
        int tmp1 = getc(tokenizer->inStream);
        if (tmp1 == '\n') tokenizer->lineNumber++;

        while (true) {
          int tmp2 = getc(tokenizer->inStream);
          if (tmp2 == '\n') tokenizer->lineNumber++;
          if (tmp1 == '*' && tmp2 == '/') break;
          tmp1 = tmp2;
        };

        continue;
      } else {
        ungetc(chrSecond, tokenizer->inStream);
      }
    }

    if (chr == '"') {
      StringBuilder *tmpSb = new_sb();

      int tmp;
      while ((tmp = getc(tokenizer->inStream)) != '"') {
        sb_add(tmpSb, tmp);
      };

      Token *token = new_token(STRING_CONST);
      token->stringValue = sb_get(tmpSb);
      add_token(tokenizer, token);

      free(tmpSb);
      break;
    }

    if (strchr("{}()[].,;+-*/&|<>=~", chr) != NULL) {
      Token *token = new_token(SYMBOL);
      token->symbol = chr;
      add_token(tokenizer, token);
      break;
    }

    // identifier or keyword
    if (isalpha(chr) || chr == '_') {
      StringBuilder *tmpStr = new_sb();
      int tmp = chr;
      while (isalpha(tmp) || tmp == '_' || isdigit(tmp)) {
        sb_add(tmpStr, tmp);
        tmp = getc(tokenizer->inStream);
      }

      char *str = sb_get(tmpStr);
      KeyWord curKeyWord = (KeyWord) map_geti(keywords_table, str, -1);

      Token *token;
      // is_equal_to if a keyword
      if (curKeyWord != -1) {
        token = new_token(KEYWORD);
        token->keyword = curKeyWord;
      } else {
        token = new_token(IDENTIFIER);
        token->identifier = str;
      }

      add_token(tokenizer, token);

      free(tmpStr);
      ungetc(tmp, tokenizer->inStream);
      break;
    }

    if (isdigit(chr)) {
      StringBuilder *tmpSb = new_sb();

      int tmp = chr;
      while (isdigit(tmp)) {
        sb_add(tmpSb, tmp);
        tmp = getc(tokenizer->inStream);
      };

      Token *token = new_token(INT_CONST);
      token->intValue = sb_get(tmpSb);
      add_token(tokenizer, token);

      free(tmpSb);
      ungetc(tmp, tokenizer->inStream);
      break;
    }

    if (isspace(chr)) {
      continue;
    }
  }
}

static Map *new_keyword_map() {
  Map *map = new_map();
  map_puti(map, "class", CLASS);
  map_puti(map, "constructor", CONSTRUCTOR);
  map_puti(map, "function", FUNCTION);
  map_puti(map, "method", METHOD);
  map_puti(map, "field", FIELD);
  map_puti(map, "static", STATIC);
  map_puti(map, "var", VAR);
  map_puti(map, "int", INT);
  map_puti(map, "char", CHAR);
  map_puti(map, "boolean", BOOLEAN);
  map_puti(map, "void", VOID);
  map_puti(map, "true", TRUE);
  map_puti(map, "false", FALSE);
  map_puti(map, "null", cNULL);
  map_puti(map, "this", THIS);
  map_puti(map, "let", LET);
  map_puti(map, "do", DO);
  map_puti(map, "if", IF);
  map_puti(map, "else", ELSE);
  map_puti(map, "while", WHILE);
  map_puti(map, "return", RETURN);
  return map;
}

char *keyword_to_string(KeyWord keyWord) {
  switch (keyWord) {
    case CLASS:
      return "class";
    case STATIC:
      return "static";
    case FIELD:
      return "field";
    case INT:
      return "int";
    case CHAR:
      return "char";
    case BOOLEAN:
      return "boolean";
    case CONSTRUCTOR:
      return "constructor";
    case FUNCTION:
      return "function";
    case METHOD:
      return "method";
    case VOID:
      return "void";
    case VAR:
      return "var";
    case LET:
      return "let";
    case IF:
      return "if";
    case ELSE:
      return "else";
    case WHILE:
      return "while";
    case DO:
      return "do";
    case RETURN:
      return "return";
    case cNULL:
      return "null";
    case THIS:
      return "this";
    case TRUE:
      return "true";
    case FALSE:
      return "false";
    default:
      xprintf("%i is not specified in keyword_to_string", keyWord);
      exit(EXIT_FAILURE);
  }
}

KeywordConst keyword_to_keywordConst(KeyWord keyWord) {
  switch (keyWord) {
    case cNULL:
      return KC_NULL;
    case THIS:
      return KC_THIS;
    case TRUE:
      return KC_TRUE;
    case FALSE:
      return KC_FALSE;
    default:
      xprintf("%s is not specified in keyword_to_keywordConst", keyword_to_string(keyWord));
      exit(EXIT_FAILURE);
  }
}

bool is_equal_to(int actual, int expected) {
  return expected == actual;
}

bool is_one_of(int actual, int nOfElements, ...) {
  va_list argp;
  va_start(argp, nOfElements);

  while (nOfElements--) {
    int expected = va_arg(argp, int);
    if (actual == expected) {
      va_end(argp);
      return true;
    }
  }
  va_end(argp);

  return false;
}

bool is_int(Tokenizer *tokenizer) {
  return is_equal_to(get_token_type(tokenizer), INT_CONST);
}

bool is_str(Tokenizer *tokenizer) {
  return is_equal_to(get_token_type(tokenizer), STRING_CONST);
}

bool is_identifier(Tokenizer *tokenizer) {
  return is_equal_to(get_token_type(tokenizer), IDENTIFIER);
}

bool is_class(Tokenizer *tokenizer) {
  return is_equal_to(get_keyword(tokenizer), CLASS);
}

bool is_class_var_dec(Tokenizer *tokenizer) {
  return is_one_of(get_keyword(tokenizer), 2, STATIC, FIELD);
}

bool is_this_symbol(Tokenizer *tokenizer, char expected) {
  return is_equal_to(get_symbol(tokenizer), expected);
}

bool is_subroutine(Tokenizer *tokenizer) {
  return is_one_of(get_keyword(tokenizer), 3, CONSTRUCTOR, FUNCTION, METHOD);
}

bool is_type(Tokenizer *tokenizer) {
  return is_identifier(tokenizer) || is_one_of(get_keyword(tokenizer), 3, INT, CHAR, BOOLEAN);
}

bool is_var_dec(Tokenizer *tokenizer) {
  return is_equal_to(get_keyword(tokenizer), VAR);
}

bool is_keyword_constant(Tokenizer *tokenizer) {
  return is_one_of(get_keyword(tokenizer), 4, TRUE, FALSE, cNULL, THIS);
}

bool is_op(Tokenizer *tokenizer) {
  return is_one_of(get_symbol(tokenizer), 9, '+', '-', '*', '/', '&', '|', '<', '>', '=');
}

bool is_unary_op(Tokenizer *tokenizer) {
  return is_one_of(get_symbol(tokenizer), 2, '-', '~');
}

bool is_term(Tokenizer *tokenizer) {
  return is_one_of(get_token_type(tokenizer), 2, INT_CONST, STRING_CONST)
         || is_keyword_constant(tokenizer)
         || is_identifier(tokenizer)
         || is_unary_op(tokenizer)
         || is_this_symbol(tokenizer, '(');
}

void raise_error(Tokenizer *tokenizer) {
  xprintf("Wrong token at line %i\n", tokenizer->lineNumber);
  xprintf("TokenType %i", get_token_type(tokenizer));
  exit(EXIT_FAILURE);
}

char *expect_identifier(Tokenizer *tokenizer) {
  if (is_identifier(tokenizer)) {
    char *identifier = get_identifier(tokenizer);
    advance(tokenizer);
    return identifier;
  }
  raise_error(tokenizer);
}


char *expect_class(Tokenizer *tokenizer) {
  if (is_class(tokenizer)) {
    advance(tokenizer);
    return "class";
  }
  raise_error(tokenizer);
}

char expect_symbol(Tokenizer *tokenizer, char symbol) {
  if (is_this_symbol(tokenizer, symbol)) {
    advance(tokenizer);
    return symbol;
  }
  raise_error(tokenizer);
}

KeyWord expect_keyword_n(Tokenizer *tokenizer, int n, ...) {
  va_list expectedArgs;
  va_start(expectedArgs, n);;
  KeyWord actualKeyWord = get_keyword(tokenizer);
  while (n--) {
    int expected = va_arg(expectedArgs, int);
    if (actualKeyWord == expected) {
      va_end(expectedArgs);
      advance(tokenizer);
      return actualKeyWord;
    }
  }

  va_end(expectedArgs);
  raise_error(tokenizer);
}

char *expect_type(Tokenizer *tokenizer) {
  // 'int' | 'char' | 'boolean' | className
  if (get_identifier(tokenizer)) {
    char *identifier = get_identifier(tokenizer);
    advance(tokenizer);
    return identifier;
  }

  KeyWord keyWord = get_keyword(tokenizer);
  switch (keyWord) {
    case INT:
    case CHAR:
    case BOOLEAN:
      advance(tokenizer);
      return keyword_to_string(keyWord);
    default:
      raise_error(tokenizer);
  }
}