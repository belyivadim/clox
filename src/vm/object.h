#ifndef __CLOX_OBJECT_H__
#define __CLOX_OBJECT_H__

#include "common.h"
#include "value.h"
#include "vm/chunk.h"

/// Object's kind getter
#define OBJ_KIND(v)       (AS_OBJ(v)->kind)

/// Object's kind identifiers
#define IS_CLOSURE(v)     is_obj_kind(v, OBJ_CLOSURE)
#define IS_FUNCTION(v)    is_obj_kind(v, OBJ_FUNCTION)
#define IS_NATIVE(v)      is_obj_kind(v, OBJ_NATIVE)
#define IS_STRING(v)      is_obj_kind(v, OBJ_STRING)

/// Object's accessors
#define AS_CLOSURE(v)     ((ObjClosure*)AS_OBJ(v))
#define AS_FUNCTION(v)    ((ObjFunction*)AS_OBJ(v))
#define AS_NATIVE(v)      (((ObjNative*)AS_OBJ(v))->pfun)
#define AS_NATIVE_OBJ(v)  ((ObjNative*)AS_OBJ(v))
#define AS_STRING(v)      ((ObjString*)AS_OBJ(v))
#define AS_CSTRING(v)     (((ObjString*)AS_OBJ(v))->chars)



/// Represents all the possible tags of object types
typedef enum {
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE
} ObjKind;

/// Represents heap allocated object in VM
struct Obj {
  /// Tag for kind of the object
  ObjKind kind;

  /// Pointer to the next heap allocated object
  struct Obj *next;
  
  /// Is it marked by GC
  bool is_marked;
};

/// Represents first class function in Lox
typedef struct {
  /// Object field
  Obj obj;

  /// Chunk of body of the function
  Chunk chunk;

  /// Name of the function
  const ObjString *name;

  /// Number of parameters the function expects
  i32 arity;

  /// Number of upvalues in function
  i32 upvalue_count;
} ObjFunction;

/// Represents pointer to native clox function 
///
/// @param arg_count: number of expected arguments
/// @param args: pointer to the first argument
/// @return Value, value returned by native function converted into clox Value
typedef Value (*NativeFn)(i32 arg_count, Value *args);

/// Represents native clox function
typedef struct {
  /// Object field
  Obj obj;

  /// Pointer to the native function
  NativeFn pfun;

  /// Number of parameters the function expects
  i32 arity;
} ObjNative;

/// Represents clox string object
struct ObjString {
  /// Object field
  Obj obj;

  /// The length of the chars array
  u32 length;

  /// Actual representation of the string
  char *chars;

  /// cache for the hash value of string pointed by chars
  u32 hash;
};

/// Represents runtime upvalue
typedef struct ObjUpvalue {
  /// Object field
  Obj obj;

  /// Pointer to the location of the upvalue
  Value *location;

  /// Closed Value, will be moved here, when it is discarded from the VM's stack
  Value closed;

  /// Intrusive LL, pointer open upvalues
  struct ObjUpvalue *next;
} ObjUpvalue;

/// Runtime representation of function with captured enviroment
typedef struct {
  /// Object field
  Obj obj;

  /// Pointer to the compiled function
  ObjFunction *pfun;

  /// Array of arrays of upvalues that closure encloses
  ObjUpvalue **upvalues;

  /// Number of active upvalues
  i32 upvalue_count;
} ObjClosure;


/// Check is the value is object, 
/// if it is check if object's kind corresponds to kind param
///
/// @param value: value to be checked
/// @param kind: expected kind of the object
/// @return bool
static inline bool is_obj_kind(Value value, ObjKind kind) {
  return IS_OBJ(value) && AS_OBJ(value)->kind == kind;
}

/// Copies chars array into string object,
/// both copied chars array and string object are heap-allocated objects
/// and need to be freed after usage
///
/// @param chars: pointer to the c-string to be copied
/// @param length: the length of the chars array
/// @return ObjString
const ObjString *string_copy(const char *chars, u32 length);

/// Creates a string object from chars array, takes ownership on chars param
///
/// @param chars: an array of characters to create a string based on
/// @param length: a length of the array
/// @return ObjString*, pointer to the newly allocated string object
const ObjString *string_create(char *chars, u32 length);

/// Creates new function object with 0 initialized fields
///
/// @return ObjFunction*, pointer to the newly allocated function,
///   passes ownership to the caller
ObjFunction *function_create();

/// Creates new native object
///
/// @param pfun: pointer to the function that will be called
///   by call to native function
/// @param arity: number of arguments pfun expects
/// @return ObjNative*, pointer to the newly allocated ObjNative,
///   passes ownership to the caller
ObjNative *native_create(NativeFn pfun, i32 arity);

/// Creates new closure object
///
/// @param pfun: pointer to the function 
///   based on which closure should be created
/// @return ObjClosure* , pointer to the newly allocated ObjClosure,
///   passes ownership to the caller
ObjClosure *closure_create(ObjFunction *pfun);

/// Creates new upvalue object
///
/// @param pslot: pointer where Value is located
/// @return ObjUpvalue*, pointer to the newly allocated ObjUpvalue,
///   passes ownership to the caller
ObjUpvalue *upvalue_create(Value *pslot);

/// Prints the value param as an object
///
/// @param value: value to be printed
/// @return void
void object_print(Value value);

const char *object_kind_to_string(ObjKind kind);

#endif // !__CLOX_OBJECT_H__
