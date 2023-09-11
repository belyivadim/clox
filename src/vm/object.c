#include <stdio.h>
#include <string.h>

#include "../utils/memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(T, obj_kind) \
  (T*)object_allocate(sizeof(T), obj_kind)

/// Allocates new ObjString based on chars and length param
///
/// @param chars: pointer to the heap allocated array of chars
/// @param length: length of the chars array
/// @return ObjString*, pointer to newly allocated string object
static ObjString *string_allocate(char *chars, u32 length, u32 hash);


/// Allocates new Obj of kind param
///
/// @param size: size of the entire object to allocate
/// @param kind: kind of the object to allocate
/// @return Obj*, pointer to newly allocated object
static Obj* object_allocate(usize size, ObjKind kind);

static u32 string_hash(const char *key, u32 length);

ObjString *string_copy(const char *chars, u32 length) {
  u32 hash = string_hash(chars, length);

  char * heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';

  return string_allocate(heap_chars, length, hash);
}

ObjString *string_create(char *chars, u32 length) {
  u32 hash = string_hash(chars, length);
  return string_allocate(chars, length, hash);
}

static ObjString *string_allocate(char *chars, u32 length, u32 hash) {
  ObjString* str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  str->length = length;
  str->chars = chars;
  str->hash = hash;

  return str;
}

static u32 string_hash(const char *key, u32 length) {
  u32 hash = 2166136261u;

  for (u32 i = 0; i < length; ++i) {
    hash ^= key[i];
    hash *= 16777619;
  }

  return hash;
}

static Obj* object_allocate(usize size, ObjKind kind) {
  Obj *pobj = (Obj*)reallocate(NULL, 0, size);
  pobj->kind = kind;

  Vm* vm = vm_instance();

  pobj->next = vm->objects;
  vm->objects = pobj;

  return pobj;
}

void object_print(Value value) {
  switch (OBJ_KIND(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}
