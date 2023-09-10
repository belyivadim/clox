#ifndef __CLOX_VALUE_H__
#define __CLOX_VALUE_H__

#include "common.h"

/// Represents all kind of values supported by VM
typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER
} ValueKind;

/// Represents a value in VM
typedef struct {
  /// Access to the value variants
  union {
    bool boolean;
    double number;
  } as;

  /// Kind of the currently stored value
  ValueKind kind;
} Value;

/// Macros to initialize/assign Value
#define BOOL_VAL(v)       ((Value){.as = {.boolean = v}, .kind = VAL_BOOL})
#define NIL_VAL           ((Value){.as = {.number = 0}, .kind = VAL_NIL})
#define NUMBER_VAL(v)     ((Value){.as = {.number = v}, .kind = VAL_NUMBER})

/// Macros to access Value
#define AS_BOOL(v)        ((v).as.boolean)
#define AS_NUMBER(v)      ((v).as.number)

/// Macros to identify Value
#define IS_BOOL(v)        (VAL_BOOL == (v).kind)
#define IS_NIL(v)         (VAL_NIL == (v).kind)
#define IS_NUMBER(v)      (VAL_NUMBER == (v).kind)

/// Represents a dynamic array of the VM's values
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

/// Compares two values.
/// Values are not equal if they are of different kind.
///
/// @params lhs, rhs: values to compare
/// @return bool
bool values_equal(Value lhs, Value rhs);

#endif // !__CLOX_VALUE_H__
