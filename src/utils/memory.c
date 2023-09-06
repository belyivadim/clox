#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

void *reallocate(void *ptr, usize old_size, usize new_size) {
  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);

  if (result == NULL) {
    fputs("[reallocate]: not enough memory!", stderr);
    exit(1);
  }

  return result;
}

