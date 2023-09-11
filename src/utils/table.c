#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "../vm/object.h"
#include "table.h"


#define TABLE_MAX_LOAD .75



/// Looking up for an entry with key in entries array
///
/// @param entries: pointer to the array to look in
/// @param capacity: capacity of the entries array
/// @param key: pointer to the key to look up entry by
/// @return Entry*, pointer to the found Entry
///   if key in found entry is NULL, then entry was not found,
///   and the nearest empty bucket for the key is returned
static Entry *find_entry(Entry* entries, i32 capacity, const ObjString *key);

/// Adjustes capacity of table's underlying array of entries to the desirable capacity
/// by reallocating it,
/// inserts all entries from old array to the new one,
/// frees old array
///
/// @param table: table to adjust
/// @param capacity: desirable capacity
static void adjust_capacity(Table *table, i32 capacity);


void table_init(Table *table) {
  assert(NULL != table);
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void table_free(Table *table) {
  assert(NULL != table);
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table_init(table);
}

bool table_set(Table* table, const ObjString *key, Value value) {
  assert(NULL != table);

  if (table->capacity * TABLE_MAX_LOAD <= table->count) {
    i32 capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  Entry *pentry = find_entry(table->entries, table->capacity, key);

  bool is_new_key = NULL == pentry->key;
  table->count += is_new_key;

  pentry->key = key;
  pentry->value = value;

  return is_new_key;
}


static Entry *find_entry(Entry* entries, i32 capacity, const ObjString *key) {
  assert(NULL != entries);
  assert(NULL != key);

  u32 index = key->hash % capacity;

  for (;;) {
    Entry *entry = entries + index;

    if (key == entry->key || NULL == entry->key) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void adjust_capacity(Table *table, i32 capacity) {
  assert(NULL != table);

  Entry *entries = ALLOCATE(Entry, capacity);

  for (i32 i = 0; i < capacity; ++i) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
  
  for (i32 i = 0; i < table->capacity; ++i) {
    Entry *entry = table->entries + i;
    if (NULL == entry->key) continue;

    Entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

void table_add_all(Table* dest, const Table *src) {
  assert(NULL != dest);
  assert(NULL != src);

  for (i32 i = 0; i < src->capacity; ++i) {
    const Entry *entry = src->entries + i;
    if (NULL != entry->key) {
      table_set(dest, entry->key, entry->value);
    }
  }
}
