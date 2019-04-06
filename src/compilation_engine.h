

#ifndef COMPILER_COMPILATION_ENGINE_H
#define COMPILER_COMPILATION_ENGINE_H

#include <stdio.h>
#include "util.h"
#include "lexer.h"
#include "symbol_table.h"
#include "vm_writer.h"
#include "parser.h"

typedef struct {
  VMwriter *writer;
  Class *ast;
  Function *curFunc;
  int labelCounter;
} CompilationEngine;

CompilationEngine *new_engine(char *fileName, Class *class);
void compile_file(CompilationEngine *engine);

#endif //COMPILER_COMPILATION_ENGINE_H
