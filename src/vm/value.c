#include <stdio.h>
#include <memory.h>

#include "utils/memory.h"
#include "value.h"
#include "object.h"

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
    case VAL_OBJ: object_print(value); break;
  }
}

bool values_equal(Value lhs, Value rhs) {
  if (lhs.kind != rhs.kind) return false;

  switch (lhs.kind) {
    case VAL_BOOL: return AS_BOOL(lhs) == AS_BOOL(rhs);
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(lhs) == AS_NUMBER(rhs);
    case VAL_OBJ: return AS_OBJ(lhs) == AS_OBJ(rhs);

    default:
      return false; // unreachable
  }
}
