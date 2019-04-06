

#ifndef COMPILER_SYMBOL_TABLE_H
#define COMPILER_SYMBOL_TABLE_H

#include "util.h"
#include "lexer.h"

#define NO_IDENTIFIER -1

typedef enum {
  KIND_STATIC,
  KIND_FIELD,
  KIND_ARG,
  KIND_VAR,
  KIND_NONE = -1
} Kind;

typedef struct {
  char *type;
  Kind kind;
  int index;
} Properties;

typedef struct {
  Map *table;
  int staticIndex;
  int fieldIndex;
  int argIndex;
  int varIndex;
} SymbolTable;

SymbolTable *init_table();
void define(SymbolTable *symbolTable, char *name, char *type, Kind kind);
int varCount(SymbolTable *symbolTable, Kind kind);
Kind kindOf(SymbolTable *symbolTable, char *name);
char *typeOf(SymbolTable *symbolTable, char *name);
int indexOf(SymbolTable *symbolTable, char *name);
Kind transformToKind(KeyWord keyword);

void print_symbol_table(SymbolTable *symbolTable);

#endif //COMPILER_SYMBOL_TABLE_H
