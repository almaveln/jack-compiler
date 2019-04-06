

#include <stdio.h>
#include <zconf.h>
#include <string.h>
#include <stdlib.h>
#include "vm_writer.h"
#include "util.h"

const char *SEGMENT_STRING[] = {
    "constant", "argument", "local", "static", "this", "that", "pointer", "temp"
};

const char *COMMAND_STRING[] = {
    "add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not"
};


static void emit(VMwriter *writer, char *fmt, int n, ...) {
  fprintf(writer->out, "%*s", writer->indentation, "");

  va_list argp;
  va_start(argp, n);

  while (n--) {
    char *elem = va_arg(argp, char*);
    if (n == 0) fmt = "%s\n";
    fprintf(writer->out, fmt, elem);
  }
  va_end(argp);
}

VMwriter *init_vmWriter(char *fileName) {
  VMwriter *writer = malloc(sizeof(VMwriter));
  char outF[strlen(fileName) + strlen(".vm") + 1];
  strcpy(outF, fileName);
  strcat(outF, ".vm");
  writer->out = fopen(outF, "w");
  writer->indentation = 0;
  return writer;
}

void write_push_i(VMwriter *writer, Segment segment, int index) {
  char *indexStr = number_to_string(index);
  emit(writer, "%s ", 3, "push", SEGMENT_STRING[segment], indexStr);
  free(indexStr);
}

void write_push(VMwriter *writer, Segment segment, char *index) {
  emit(writer, "%s ", 3, "push", SEGMENT_STRING[segment], index);
}

void write_pop_i(VMwriter *writer, Segment segment, int index) {
  char *indexStr = number_to_string(index);
  emit(writer, "%s ", 3, "pop", SEGMENT_STRING[segment], indexStr);
  free(indexStr);
}

// UNUSED
void write_pop(VMwriter *writer, Segment segment, char *index) {
  emit(writer, "%s ", 3, "pop", SEGMENT_STRING[segment], index);
}

void write_arithmetic(VMwriter *writer, Command command) {
  emit(writer, "%s", 1, COMMAND_STRING[command]);
}

void write_label(VMwriter *writer, char *label) {
  emit(writer, "%s ", 2, "label", label);
}

void write_goto(VMwriter *writer, char *label) {
  emit(writer, "%s ", 2, "goto", label);
}

void write_if(VMwriter *writer, char *label) {
  emit(writer, "%s ", 2, "if-goto", label);
}

void write_call(VMwriter *writer, char *className, char *label, int nArgs) {
  char *nArgsStr = number_to_string(nArgs);
  emit(writer, "%s", 6, "call ", className, ".", label, " ", nArgsStr);
  free(nArgsStr);
}

void write_func(VMwriter *writer, char *className, char *name, int nLocals) {
  char *nLocalsStr = number_to_string(nLocals);
  emit(writer, "%s", 6, "function ", className, ".", name, " ", nLocalsStr);
  free(nLocalsStr);
}

void write_return(VMwriter *writer) {
  emit(writer, "%s", 1, "return");
}
