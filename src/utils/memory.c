#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "../vm/vm.h"
#include "../vm/object.h"

/// Frees Object pointed by pobj and all the resources holded by pobj
///
/// @param pobj: pointer to the object to be freed
/// @return void
static void object_free(Obj *pobj);

void *reallocate(void *ptr, usize old_size, usize new_size) {
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
  switch (pobj->kind) {
    case OBJ_STRING: {
      ObjString *pstr = (ObjString*)pobj;
      FREE_ARRAY(char, pstr->chars, pstr->length + 1);
      FREE(ObjString, pobj);
      break;
    }

    case OBJ_FUNCTION: {
      ObjFunction *pfun = (ObjFunction*)pobj;
      chunk_free(&pfun->chunk);
      FREE(ObjFunction, pobj);
      break;
    }
  }
}
