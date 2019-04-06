#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "util.h"
#include "lexer.h"
#include "compilation_engine.h"
#include "symbol_table.h"
#include "parser.h"

static char *get_basename_without_ext(char *path) {
  char *baseName = basename(path);
  char *nameWithoutExtension = sb_get((StringBuilder *) vec_get(split_by(baseName, '.'), 0));
  return nameWithoutExtension;
}

static void process_file(char *path) {
  Tokenizer *tokenizer = new_tokenizer(path);
  Class *class = build_ast(tokenizer);

  CompilationEngine *engine = new_engine(get_basename_without_ext(path), class);
  compile_file(engine);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    xprintf("A wrong number of arguments is given to the program\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < argc; i++) {
    xprintf("i: %i; argv %s\n", i, *(argv + i));
  }

  char *receivedPath = argv[1];

  if (isDir(receivedPath)) {
    struct dirent *dp;
    struct stat statbuf;

    bool isFirstFile = true;

    DIR *dir = opendir(receivedPath);

    while ((dp = readdir(dir)) != NULL) {
      StringBuilder *sb = new_sb();

      sb_concat_strings(sb, 3, receivedPath, "/", dp->d_name);
      char *entry = sb_get(sb);

      if (stat(entry, &statbuf) == -1)
        continue;

      if (is_reg_file(entry) && has_jack_extension(entry)) {
        if (isFirstFile) {
          isFirstFile = false;
        }
        process_file(entry);
      }

      free(sb);
    }

    if (isFirstFile) {
      exit(EXIT_FAILURE);
    }

    return 0;
  }

  if (is_reg_file(receivedPath) && has_jack_extension(receivedPath)) {
    process_file(receivedPath);
    return 0;
  }

  exit(EXIT_FAILURE);
}

