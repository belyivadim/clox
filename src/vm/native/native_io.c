#include <stdio.h>

#include "native_io.h"
#include "../../utils/memory.h"

Value readln_native(i32 arg_count, Value *args) {
  (void)arg_count;
  (void)args;

  usize i = 0;
  i32 c;
  char *buff = NULL;
  usize buff_capacity = MEM_MIN_CAPACITY;
  buff = GROW_ARRAY(char, buff, 0, buff_capacity);

  while ((c = getchar()) != '\n' && EOF != c) {
    if (i == buff_capacity) {
      usize old_capacity = buff_capacity;
      buff_capacity = GROW_CAPACITY(buff_capacity);
      buff = GROW_ARRAY(char, buff, old_capacity, buff_capacity);
    }

    buff[i++] = c;
  }

  if (i == buff_capacity) {
    buff_capacity += 1;
    buff = GROW_ARRAY(char, buff, buff_capacity - 1, buff_capacity);
  }

  buff[i] = '\0';

  return OBJ_VAL(string_create(buff, i));
}
