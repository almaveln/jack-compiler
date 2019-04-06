
#include <stdbool.h>

#ifndef VIRTUAL_MACHINE_COMMON_H
#define VIRTUAL_MACHINE_COMMON_H

typedef struct {
  char *data;
  int capacity;
  int len;
} StringBuilder;

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;


StringBuilder *new_sb(void);
void sb_add(StringBuilder *sb, char c);
void sb_append(StringBuilder *sb, char *s);
void sb_append_n(StringBuilder *sb, char *s, int len);
char *sb_get(StringBuilder *sb);
void sb_concat_strings(StringBuilder *sb, int nOfStrings, ...);
void sb_append_i(StringBuilder *sb, int numb);

Vector *new_vec();
void vec_push(Vector *v, void *elem);
void *vec_get(Vector *v, int index);

Map *new_map(void);
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key, int default_);

void xprintf(char *format, ...);
char *number_to_string(int number);

Vector *split_by(char *str, char delim);

bool has_jack_extension(const char *fileName);
bool isDir(char *path);
bool is_reg_file(char *path);


#endif //VIRTUAL_MACHINE_COMMON_H
