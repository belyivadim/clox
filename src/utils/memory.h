#ifndef __CLOX_MEMORY_H__
#define __CLOX_MEMORY_H__

#include "common.h"
#include "../vm/value.h"

enum {
  MEM_MIN_CAPACITY = 8,
  MEM_GROW_FACTOR = 2
};

/// returns capacity multiplied by MEM_GROW_FACTOR
/// or MEM_MIN_CAPACITY in case if the initial capacity less than MEM_MIN_CAPACITY
#define GROW_CAPACITY(capacity) \
  ((capacity) < MEM_MIN_CAPACITY ? \
    MEM_MIN_CAPACITY : \
    (capacity) * MEM_GROW_FACTOR)

/// realocates array pointed by ptr with new_size
#define GROW_ARRAY(T, ptr, old_count, new_count) \
  (T*)reallocate(ptr, sizeof(T) * (old_count), sizeof(T) * (new_count))

/// frees array pointed by @ptr
#define FREE_ARRAY(T, ptr, old_count) \
  reallocate(ptr, sizeof(T) * old_count, 0)

#define FREE(T, ptr) reallocate(ptr, sizeof(T), 0)

#define ALLOCATE(T, count) \
  (T*)reallocate(NULL, 0, sizeof(T) * (count))

/// Realocates the memory at ptr with new_size
/// if realocation failed, terminates the process with exit code 1
/// if new_size is 0, frees the memory pointed by ptr
///
/// @param ptr pointer to the memory to be reallocated
/// @param old_size old size in bytes
/// @param new_size new size in bytes
/// @return void*, pointer to newly allocated memory or NULL in case if new_size is 0
void *reallocate(void *ptr, usize old_size, usize new_size);

/// Runs garbage collector (uses mark-and-swap algorithm)
///
/// @return void
void collect_garbage();

/// GC marks value
void mark_value(Value value);

/// GC marks object
void mark_object(Obj *pobj);

/// Frees all heap-allocated objects in virtual machine singleton
///
/// @return void
void free_objects();


#endif // !__CLOX_MEMORY_H__
