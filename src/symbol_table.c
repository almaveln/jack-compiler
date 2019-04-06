

#include <wchar.h>
#include <stdlib.h>
#include "symbol_table.h"
#include "lexer.h"

const static char *kinds[] = {
    "static", "field", "arg", "var", "none"
};

static int incr_kind_index(SymbolTable *symbolTable, Kind kind);
static Properties *create_props(char *type, Kind kind, int index);

SymbolTable *init_table() {
  SymbolTable *symbolTable = malloc(sizeof(SymbolTable));
  symbolTable->table = new_map();
  symbolTable->staticIndex = -1;
  symbolTable->varIndex = -1;
  symbolTable->fieldIndex = -1;
  symbolTable->argIndex = -1;
  return symbolTable;
}

void define(SymbolTable *sTable, char *name, char *type, Kind kind) {
  int newIndex = incr_kind_index(sTable, kind);
  Properties *properties = create_props(type, kind, newIndex);
  map_put(sTable->table, name, properties);
}

int varCount(SymbolTable *symbolTable, Kind kind) {
  switch (kind) {
    case KIND_STATIC:
      return symbolTable->staticIndex + 1;
    case KIND_FIELD:
      return symbolTable->fieldIndex + 1;
    case KIND_ARG:
      return symbolTable->argIndex + 1;
    case KIND_VAR:
      return symbolTable->varIndex + 1;
    default:
      xprintf("define in symbol_table; unspecified kind");
      exit(EXIT_FAILURE);
  }
}

Kind kindOf(SymbolTable *symbolTable, char *name) {
  Properties *properties = map_get(symbolTable->table, name);
  return properties == NULL ? KIND_NONE : properties->kind;
}

char *typeOf(SymbolTable *symbolTable, char *name) {
  Properties *properties = map_get(symbolTable->table, name);
  return properties == NULL ? NULL : properties->type;
}

int indexOf(SymbolTable *symbolTable, char *name) {
  Properties *properties = map_get(symbolTable->table, name);
  return properties == NULL ? NO_IDENTIFIER : properties->index;
}

Kind transformToKind(KeyWord keyword) {
  switch (keyword) {
    case STATIC:
      return KIND_STATIC;
    case FIELD:
      return KIND_FIELD;
    case VAR:
      return KIND_VAR;
    default:
      xprintf("transformToKind; incorrect keyword %i", keyword);
      exit(EXIT_FAILURE);
  }
}

void print_symbol_table(SymbolTable *symbolTable) {
  Vector *keys = symbolTable->table->keys;
  for (int i = 0; i < keys->len; i++) {
    char *key = vec_get(keys, i);
    Properties *props = (Properties*) map_get(symbolTable->table, key);
    xprintf("key: %s, value: {index: %i, kind: %s, type: %s}\n", key, props->index, kinds[props->kind], props->type);
  }
}

static int incr_kind_index(SymbolTable *symbolTable, Kind kind) {
  switch (kind) {
    case KIND_STATIC:
      return ++symbolTable->staticIndex;
    case KIND_FIELD:
      return ++symbolTable->fieldIndex;
    case KIND_ARG:
      return ++symbolTable->argIndex;
    case KIND_VAR:
      return ++symbolTable->varIndex;
    default:
      xprintf("define in symbol_table; unspecified kind");
      exit(EXIT_FAILURE);
  }
}

static Properties *create_props(char *type, Kind kind, int index) {
  Properties *properties = malloc(sizeof(Properties));
  properties->type = type;
  properties->kind = kind;
  properties->index = index;
  return properties;
}
