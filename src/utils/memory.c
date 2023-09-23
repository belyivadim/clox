#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "../vm/vm.h"
#include "../vm/object.h"
#include "../frontend/compiler.h"

#ifdef DEBUG_LOG_GC
#include "../vm/debug.h"
#endif // !DEBUG_LOG_GC

#define GC_HEAP_GROW_FACTOR 2

/// Frees Object pointed by pobj and all the resources holded by pobj
///
/// @param pobj: pointer to the object to be freed
/// @return void
static void object_free(Obj *pobj);

/// GC functions
static void mark_roots();
static void trace_references();
static void blacken_object(Obj *pobj);
static void mark_array(ValueArray *arr);
static void sweep();

void *reallocate(void *ptr, usize old_size, usize new_size) {
  Vm *vm = vm_instance();

  vm->bytes_allocated += new_size - old_size;

  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif // !DEBUG_STRESS_GC
    if (vm->bytes_allocated > vm->next_gc) {
      collect_garbage();
    }
  }


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

  free(vm->gray_stack);
}

static void object_free(Obj *pobj) {
#ifdef DEBUG_LOG_GC
  printf(COLOR_FG_YELLOW "%p is about to be freed, kind (%s)\n" COLOR_FG_RESET,
         (void*)pobj, object_kind_to_string(pobj->kind));
#endif // !DEBUG_LOG_GC


  switch (pobj->kind) {
    case OBJ_CLASS: {
      FREE(ObjClass, pobj);
      break;
    }

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
  Vm *vm = vm_instance();

#ifdef DEBUG_LOG_GC
  puts(COLOR_FG_YELLOW "-- gc begin");
  usize before = vm->bytes_allocated;
#endif // !DEBUG_LOG_GC

  mark_roots();
  trace_references();
  table_remove_white(&vm->strings);
  sweep();

  vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  puts("-- gc end" COLOR_FG_RESET);
  printf("   collected %ld bytes (from %ld to %ld) next at %ld\n",
         before - vm->bytes_allocated, before, vm->bytes_allocated, vm->next_gc);
#endif // !DEBUG_LOG_GC

}

static void mark_roots() {
  Vm *vm = vm_instance();
  for (Value *slot = vm->stack; slot < vm->stack_top; ++slot) {
    mark_value(*slot);
  }

  for (i32 i = 0; i < vm->frame_count; ++i) {
    mark_object((Obj*)vm->frames[i].pclosure);
  }

  for (ObjUpvalue *upvalue = vm->open_upvalues;
       NULL != upvalue;
       upvalue = upvalue->next) {
    mark_object((Obj*)upvalue);
  }

  mark_table(&vm->globals);
  mark_compiler_roots();
}

void mark_value(Value value) {
  if (!IS_OBJ(value)) return;
  mark_object(AS_OBJ(value));
}

void mark_object(Obj *pobj) {
  if (NULL == pobj || pobj->is_marked) return;

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)pobj);
  value_print(OBJ_VAL(pobj));
  puts("");
#endif // !DEBUG_LOG_GC

  pobj->is_marked = true;

  Vm *vm = vm_instance();

  if (vm->gray_count == vm->gray_capacity) {
    vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
    vm->gray_stack = realloc(vm->gray_stack, sizeof(Obj*) * vm->gray_capacity);

    if (NULL == vm->gray_stack) {
      fputs("[realloc]: not enough memory while realocating gray_stack!", stderr);
      exit(1);
    }
  }

  vm->gray_stack[vm->gray_count++] = pobj;
}

static void trace_references() {
  Vm *vm = vm_instance();
  while (vm->gray_count > 0) {
    Obj *pobj = vm->gray_stack[--vm->gray_count];
    blacken_object(pobj);
  }
}

static void mark_array(ValueArray *arr) {
  for (i32 i = 0; i < arr->count; ++i) {
    mark_value(arr->values[i]);
  }
}

static void blacken_object(Obj *pobj) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)pobj);
  value_print(OBJ_VAL(pobj));
  puts("");
#endif // !DEBUG_LOG_GC

  switch (pobj->kind) {
    case OBJ_CLASS: {
      ObjClass *pcls = (ObjClass*)pobj;
      mark_object((Obj*)pcls->name);
      break;
    }

    case OBJ_CLOSURE: {
      ObjClosure *pclosure = (ObjClosure*)pobj;
      mark_object((Obj*)pclosure->pfun);
      for (i32 i = 0; i < pclosure->upvalue_count; ++i) {
        mark_object((Obj*)pclosure->upvalues[i]);
      }
      break;
    }

    case OBJ_FUNCTION: {
      ObjFunction *pfun = (ObjFunction*)pobj;
      mark_object((Obj*)pfun->name);
      mark_array(&pfun->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      mark_value(((ObjUpvalue*)pobj)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}


static void sweep() {
  Vm *vm = vm_instance();
  Obj *prev = NULL;
  Obj *pobj = vm->objects;

  while (NULL != pobj) {
    if (pobj->is_marked) {
      pobj->is_marked = false;
      prev = pobj;
      pobj = pobj->next;
    } else {
      Obj *punreached = pobj;

      pobj = pobj->next;
      if (NULL != prev) {
        prev->next = pobj;
      } else {
        vm->objects = pobj;
      }

      object_free(punreached);
    }
  }
}
