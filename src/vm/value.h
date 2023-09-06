#ifndef __CLOX_VALUE_H__
#define __CLOX_VALUE_H__

#include "common.h"

typedef double Value;

typedef struct {
  Value *values;
  i32 count;
  i32 capacity;
} ValueArray;

/// Initializes all fields of ValueArray at arr to zeros
///
/// @param arr pointer to the ValueArray to be initialized
/// @return void
void value_array_init(ValueArray *arr);

/// Frees memory allocated for arr and clean up its fields (set to zeros)
///
/// @param arr pointer to the ValueArray to be freed
/// @return void
void value_array_free(ValueArray *arr);

/// Writes a value to the next avialable index in arr
///
/// @param arr pointer to the ValueArray to be written to
/// @param value a value to be written to the chunk
/// @return void
void value_array_write(ValueArray *arr, Value value);

/// Prints value to stdout
///
/// @param value a value to be printed
/// @return void
void value_print(Value value);


#endif // !__CLOX_VALUE_H__
