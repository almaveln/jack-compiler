

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include "util.h"


static const bool DEBUG = true;

void xprintf(char *format, ...) {
  if (DEBUG) {
    va_list argp;
    va_start(argp, format);
    vprintf(format, argp);
    va_end(argp);
  }
}

static int calc_mem_value_for_int(number) {
  if (number == 0)
    return sizeof(char);
  return (int) ((ceil(log10(number))) * sizeof(char));
}

char *number_to_string(int number) {
  int numberOfChars = calc_mem_value_for_int(number);
  char *str = malloc(sizeof(char) * numberOfChars + 1);
  sprintf(str, "%d", number);
  return str;
}

StringBuilder *new_sb(void) {
  StringBuilder *sb = malloc(sizeof(StringBuilder));
  sb->data = malloc(8);
  sb->capacity = 8;
  sb->len = 0;
  return sb;
}

static void sb_grow(StringBuilder *sb, int len) {
  if (sb->len + len <= sb->capacity)
    return;

  while (sb->len + len > sb->capacity)
    sb->capacity *= 2;
  sb->data = realloc(sb->data, sb->capacity);
}

void sb_add(StringBuilder *sb, char c) {
  sb_grow(sb, 1);
  sb->data[sb->len++] = c;
}

void sb_append(StringBuilder *sb, char *s) {
  sb_append_n(sb, s, strlen(s));
}

void sb_append_i(StringBuilder *sb, int numb) {
  char *s = number_to_string(numb);
  sb_append_n(sb, s, strlen(s));
  free(s);
}

void sb_appendln(StringBuilder *sb, char *s) {
  sb_append_n(sb, s, strlen(s));
  sb_append(sb, "\n");
}

void sb_concat_strings(StringBuilder *sb, int nOfStrings, ...) {
  va_list argp;
  va_start(argp, nOfStrings);

  while (nOfStrings--) {
    char *s = va_arg(argp, char*);
    sb_append_n(sb, s, strlen(s));
  }
  va_end(argp);
}

void sb_concat_strings_ln(StringBuilder *sb, int nOfStrings, ...) {
  va_list argp;
  va_start(argp, nOfStrings);

  while (nOfStrings--) {
    char *s = va_arg(argp, char*);
    sb_append_n(sb, s, strlen(s));
  }
  va_end(argp);
  sb_append(sb, "\n");
}

void sb_append_n(StringBuilder *sb, char *s, int len) {
  sb_grow(sb, len);
  memcpy(sb->data + sb->len, s, len);
  sb->len += len;
}

// NOTE: if the function is called two times, then
// there will be two null terminators
char *sb_get(StringBuilder *sb) {
  sb_add(sb, '\0');
  return sb->data;
}

char sb_get_char(StringBuilder *sb, int index) {
  return sb->data[index];
}

Vector *new_vec() {
  Vector *v = malloc(sizeof(Vector));
  v->data = malloc(sizeof(void *) * 16);
  v->capacity = 16;
  v->len = 0;
  return v;
}

void vec_push(Vector *v, void *elem) {
  if (v->len == v->capacity) {
    v->capacity *= 2;
    v->data = realloc(v->data, sizeof(void *) * v->capacity);
  }
  v->data[v->len++] = elem;
}

void *vec_get(Vector *v, int index) {
  if (index >= v->len)
    return NULL;
  return *(v->data + index);
}

// free memory if vector consists of elements that are located in the heap
// otherwise error will be given (signal 6: SIGABRT)
void free_vec_and_elements(Vector *vector) {
  for (int i = 0; i < vector->len; i++) {
    free(vec_get(vector, i));
  }
  free(vector->data);
  free(vector);
  vector = NULL;
}

Vector *split_sb_by(StringBuilder *sb, char delim) {
  Vector *vector = new_vec();

  char *sbStr = sb_get(sb);
  int last = 0;
  for (int i = 0; i < sb->len; i++) {
    char c = *(sbStr + i);

    if (c == delim || c == '\0') {
      StringBuilder *tmpSb = new_sb();
      sb_append_n(tmpSb, sbStr + last, i - last);
      //xprintf("%s\n", sb_get(tmpSb));
      //xprintf("%i\n", strlen(tmpSb->data));

      vec_push(vector, tmpSb);
      last = i + 1;
    }
  }

  return vector;
}

Vector *split_by(char *str, char delim) {
  Vector *vector = new_vec();

  int last = 0;
  int i = 0;
  char c;
  do {
    c = *(str + i);

    if (c == delim || c == '\0') {
      StringBuilder *tmpSb = new_sb();
      sb_append_n(tmpSb, str + last, i - last);
      //xprintf("%s\n", sb_get(tmpSb));
      //xprintf("%i\n", strlen(tmpSb->data));

      vec_push(vector, tmpSb);
      last = i + 1;
    }

    i++;
  } while (c != '\0');

  return vector;
}

Map *new_map(void) {
  Map *map = malloc(sizeof(Map));
  map->keys = new_vec();
  map->vals = new_vec();
  return map;
}

void map_put(Map *map, char *key, void *val) {
  vec_push(map->keys, key);
  vec_push(map->vals, val);
}

void map_puti(Map *map, char *key, int val) {
  map_put(map, key, (void *) (int) val);
}

void *map_get(Map *map, char *key) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return map->vals->data[i];
  return NULL;
}

int map_geti(Map *map, char *key, int default_) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return (int) map->vals->data[i];
  return default_;
}

bool has_jack_extension(const char *fileName) {
  int i = 0;
  while (*(fileName + i) != '\0') {
    i++;
  }

  return i > 6 && fileName[i - 5] == '.' && fileName[i - 4] == 'j' && fileName[i - 3] == 'a'
         && fileName[i - 2] == 'c' && fileName[i - 1] == 'k';
}

bool isDir(char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0)
    return false;
  return S_ISDIR(statbuf.st_mode);
}

bool is_reg_file(char *path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0)
    return false;
  return S_ISREG(statbuf.st_mode);
}