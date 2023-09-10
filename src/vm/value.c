#include <stdio.h>

#include "utils/memory.h"
#include "value.h"

void value_array_init(ValueArray *arr) {
  assert(NULL != arr);

  arr->values = NULL;
  arr->count = 0;
  arr->capacity = 0;
}

void value_array_free(ValueArray *arr) {
  assert(NULL != arr);

  FREE_ARRAY(Value, arr->values, arr->capacity);
  value_array_init(arr);
}

void value_array_write(ValueArray *arr, Value value) {
  assert(NULL != arr);

  if (arr->capacity <= arr->count) {
    i32 old_capacity = arr->capacity;
    arr->capacity = GROW_CAPACITY(old_capacity);
    arr->values = GROW_ARRAY(Value, arr->values, old_capacity, arr->capacity);
  }

  arr->values[arr->count++] = value;
}

void value_print(Value value) {
  switch (value.kind) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
  }
}
