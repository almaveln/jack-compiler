

#ifndef COMPILER_VM_WRITER_H
#define COMPILER_VM_WRITER_H

// contains info about writer
typedef struct {
  FILE *out;
  int indentation;
  char *className;
} VMwriter;

typedef enum {
  SEGMENT_CONST,
  SEGMENT_ARG,
  SEGMENT_LOCAL,
  SEGMENT_STATIC,
  SEGMENT_THIS,
  SEGMENT_THAT,
  SEGMENT_POINTER,
  SEGMENT_TEMP
} Segment;

typedef enum {
  ADD,
  SUB,
  NEG,
  EQ,
  GT,
  LT,
  AND,
  OR,
  NOT
} Command;


VMwriter *init_vmWriter(char *fileName);
void close_vmWriter(VMwriter *writer);
void write_func(VMwriter *writer, char *className, char *name, int nLocals);
void write_return(VMwriter *writer);
void write_push(VMwriter *writer, Segment segment, char *index);
void write_push_i(VMwriter *writer, Segment segment, int index);
void write_pop(VMwriter *writer, Segment segment, char *index);
void write_pop_i(VMwriter *writer, Segment segment, int index);
void write_arithmetic(VMwriter *writer, Command command);
void write_label(VMwriter *writer, char *label);
void write_goto(VMwriter *writer, char *label);
void write_if(VMwriter *writer, char *label);
void write_call(VMwriter *writer, char *className, char *label, int nArgs);

#endif //COMPILER_VM_WRITER_H
