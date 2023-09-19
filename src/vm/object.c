#include <stdio.h>
#include <string.h>

#include "../utils/memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#include "vm/debug.h"

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

static void function_print(const ObjFunction *pfun);

const ObjString *string_copy(const char *chars, u32 length) {
  u32 hash = string_hash(chars, length);

  const ObjString* interned = 
    table_find_string(&vm_instance()->strings, chars, length, hash);

  if (NULL != interned) return interned;

  char * heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';

  return string_allocate(heap_chars, length, hash);
}

const ObjString *string_create(char *chars, u32 length) {
  u32 hash = string_hash(chars, length);

  const ObjString *interned =
    table_find_string(&vm_instance()->strings, chars, length, hash);

  if (NULL != interned) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return string_allocate(chars, length, hash);
}

static ObjString *string_allocate(char *chars, u32 length, u32 hash) {
  ObjString* str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  str->length = length;
  str->chars = chars;
  str->hash = hash;

  table_set(&vm_instance()->strings, str, NIL_VAL);

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
  pobj->is_marked = false;

  Vm* vm = vm_instance();

  pobj->next = vm->objects;
  vm->objects = pobj;

#ifdef DEBUG_LOG_GC
  printf(COLOR_FG_YELLOW "%p allocated with size of %ld for %s\n" COLOR_FG_RESET, 
         (void*)pobj, size, object_kind_to_string(kind));
#endif // !DEBUG_LOG_GC

  return pobj;
}

static void function_print(const ObjFunction *pfun) {
  assert(NULL != pfun);

  if (NULL == pfun->name) {
    printf("<script>");
    return;
  }

  printf("<fun %s>", pfun->name->chars);
}

ObjFunction *function_create() {
  ObjFunction *pfun = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  pfun->arity = 0;
  pfun->upvalue_count = 0;
  pfun->name = NULL;
  chunk_init(&pfun->chunk);
  
  return pfun;
}

ObjClosure *closure_create(ObjFunction *pfun) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, pfun->upvalue_count);

  for (i32 i = 0; i < pfun->upvalue_count; ++i) {
    upvalues[i] = NULL;
  }

  ObjClosure *pclosure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  pclosure->pfun = pfun;
  pclosure->upvalues = upvalues;
  pclosure->upvalue_count = pfun->upvalue_count;
  return pclosure;
}

ObjNative *native_create(NativeFn pfun, i32 arity) {
  ObjNative *pnative = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  pnative->pfun = pfun;
  pnative->arity = arity;
  return pnative;
}

ObjUpvalue *upvalue_create(Value *pslot) {
  ObjUpvalue *pupvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  pupvalue->closed = NIL_VAL;
  pupvalue->location = pslot;
  pupvalue->next = NULL;
  return pupvalue;
}

void object_print(Value value) {
  switch (OBJ_KIND(value)) {
    case OBJ_STRING: {
      printf("%s", AS_CSTRING(value));
      break;
    }

    case OBJ_NATIVE: {
      printf("<native fun>");
      break;
    }

    case OBJ_FUNCTION: {
      function_print(AS_FUNCTION(value));
      break;
    }

    case OBJ_CLOSURE: {
      function_print(AS_CLOSURE(value)->pfun);
      break;
    }

    case OBJ_UPVALUE: {
      printf("upvalue");
      break;
    }
  }
}

const char *object_kind_to_string(ObjKind kind) {
  switch (kind) {
    case OBJ_CLOSURE: return "OBJ_CLOSURE";
    case OBJ_FUNCTION: return "OBJ_FUNCTION";
    case OBJ_NATIVE: return "OBJ_NATIVE";
    case OBJ_STRING: return "OBJ_STRING";
    case OBJ_UPVALUE: return "OBJ_UPVALUE";

    default:
      return "<Unknown object kind>";
  }
}
