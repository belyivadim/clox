#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "../vm/vm.h"
#include "../vm/object.h"

#ifdef DEBUG_LOG_GC
#include "../vm/debug.h"
#endif // !DEBUG_LOG_GC


/// Frees Object pointed by pobj and all the resources holded by pobj
///
/// @param pobj: pointer to the object to be freed
/// @return void
static void object_free(Obj *pobj);

static void mark_roots();

void *reallocate(void *ptr, usize old_size, usize new_size) {
#ifdef DEBUG_STRESS_GC
  if (new_size > old_size) {
    collect_garbage();
  }
#endif // !DEBUG_STRESS_GC

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

void free_objects() {
  Vm *vm = vm_instance();
  Obj *pobj = vm->objects;
  while (NULL != pobj) {
    Obj *pnext = pobj->next;
    object_free(pobj);
    pobj = pnext;
  }
}

static void object_free(Obj *pobj) {
#ifdef DEBUG_LOG_GC
  printf(COLOR_FG_YELLOW "%p is about to be freed, kind (%s)\n" COLOR_FG_RESET,
         (void*)pobj, object_kind_to_string(pobj->kind));
#endif // !DEBUG_LOG_GC


  switch (pobj->kind) {
    case OBJ_STRING: {
      ObjString *pstr = (ObjString*)pobj;
      FREE_ARRAY(char, pstr->chars, pstr->length + 1);
      FREE(ObjString, pobj);
      break;
    }

    case OBJ_NATIVE: {
      FREE(ObjNative, pobj);
      break;
    }

    case OBJ_FUNCTION: {
      ObjFunction *pfun = (ObjFunction*)pobj;
      chunk_free(&pfun->chunk);
      FREE(ObjFunction, pobj);
      break;
    }

    case OBJ_CLOSURE: {
      ObjClosure *pclosure = (ObjClosure*)pobj;
      FREE_ARRAY(ObjUpvalue*, pclosure->upvalues, pclosure->upvalue_count);
      FREE(ObjClosure, pobj);
      break;
    }

    case OBJ_UPVALUE: {
      FREE(ObjUpvalue, pobj);
      break;
    }
  }
}

void collect_garbage() {
#ifdef DEBUG_LOG_GC
  puts(COLOR_FG_YELLOW "-- gc begin");
#endif // !DEBUG_LOG_GC

  mark_roots();

#ifdef DEBUG_LOG_GC
  puts("-- gc end" COLOR_FG_RESET);
#endif // !DEBUG_LOG_GC

}

static void mark_roots() {
  Vm *vm = vm_instance();
  for (Value *slot = vm->stack; slot < vm->stack_top; ++slot) {
    mark_value(*slot);
  }

  mark_table(&vm->globals);
}

void mark_value(Value value) {
  if (!IS_OBJ(value)) return;
  mark_object(AS_OBJ(value));
}

void mark_object(Obj *pobj) {
  if (NULL == pobj) return;

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)pobj);
  value_print(OBJ_VAL(pobj));
  puts("");
#endif // !DEBUG_LOG_GC

  pobj->is_marked = true;
}
