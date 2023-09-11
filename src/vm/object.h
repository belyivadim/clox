#ifndef __CLOX_OBJECT_H__
#define __CLOX_OBJECT_H__

#include "common.h"
#include "value.h"

/// Object's kind getter
#define OBJ_KIND(v)       (AS_OBJ(v)->kind)

/// Object's kind identifiers
#define IS_STRING(v)      is_obj_kind(v, OBJ_STRING)

/// Object's accessors
#define AS_STRING(v)      ((ObjString*)AS_OBJ(v))
#define AS_CSTRING(v)     (((ObjString*)AS_OBJ(v))->chars)



/// Represents all the possible tags of object types
typedef enum {
  OBJ_STRING
} ObjKind;

/// Represents heap allocated object in VM
struct Obj {
  /// Tag for kind of the object
  ObjKind kind;
};

/// Represents clox string object
struct ObjString {
  /// Object field
  Obj obj;

  /// The length of the chars array
  u32 length;

  /// Actual representation of the string
  char *chars;
};


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
ObjString *string_copy(const char *chars, u32 length);

/// Creates a string object from chars array, takes ownership on chars param
///
/// @param chars: an array of characters to create a string based on
/// @param length: a length of the array
/// @return ObjString*, pointer to the newly allocated string object
ObjString *string_create(char *chars, u32 length);

/// Prints the value param as an object
///
/// @param value: value to be printed
/// @return void
void object_print(Value value);

#endif // !__CLOX_OBJECT_H__
