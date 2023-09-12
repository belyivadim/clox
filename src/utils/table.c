#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "../vm/object.h"
#include "../vm/value.h"
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
  table->tombstones_count = 0;
  table->entries = NULL;
}

void table_free(Table *table) {
  assert(NULL != table);
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table_init(table);
}

bool table_set(Table* table, const ObjString *key, Value value) {
  assert(NULL != table);

  if (table->capacity * TABLE_MAX_LOAD <= table->count + table->tombstones_count) {
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

bool table_get(const Table* table, const ObjString *key, Value* value) {
  assert(NULL != table);

  if (0 == table->count) return false;

  Entry *pentry = find_entry(table->entries, table->capacity, key);
  if (NULL == pentry->key) return false;

  *value = pentry->value;
  return true;
}

bool table_delete(Table *table, const ObjString *key) {
  assert(NULL != table);

  if (0 == table->count) return false;

  Entry *pentry = find_entry(table->entries, table->capacity, key);
  if (NULL == pentry->key) return false;

  // tombstone
  pentry->key = NULL;
  pentry->value = BOOL_VAL(true);
  --table->count;
  ++table->tombstones_count;

  return true;
}

const ObjString* table_find_string(const Table *table, const char *chars, u32 length, u32 hash) {
  assert(NULL != table);
  assert(NULL != chars);

  if (0 == table->count) return NULL;

  u32 index = hash % table->capacity;

  for (;;) {
    Entry *pentry = table->entries + index;

    if (NULL == pentry->key) {
      if (IS_NIL(pentry->value)) return NULL; // empty, non tombstone value is found
    } else if (pentry->key->length == length 
              && pentry->key->hash == hash
              && 0 == memcmp(pentry->key->chars, chars, length)) {
      // key is found
      return pentry->key;
    }

    index = (index + 1) % table->capacity;
  }
}

void table_add_all(Table* dest, const Table *src) {
  assert(NULL != dest);
  assert(NULL != src);

  for (i32 i = 0; i < src->capacity; ++i) {
    const Entry *pentry = src->entries + i;
    if (NULL != pentry->key) {
      table_set(dest, pentry->key, pentry->value);
    }
  }
}


static Entry *find_entry(Entry* entries, i32 capacity, const ObjString *key) {
  assert(NULL != entries);
  assert(NULL != key);

  u32 index = key->hash % capacity;
  Entry *tombstone = NULL;

  for (;;) {
    Entry *pentry = entries + index;

    if (NULL == pentry->key) {
      if (IS_NIL(pentry->value)) {
        // empty entry
        return NULL != tombstone ? tombstone : pentry;
      } else {
        // tombstone
        if (NULL == tombstone) tombstone = pentry;
      }
    } else if (key == pentry->key) {
      // the key is found
      return pentry;
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
    Entry *pentry = table->entries + i;
    if (NULL == pentry->key) continue;

    Entry *dest = find_entry(entries, capacity, pentry->key);
    dest->key = pentry->key;
    dest->value = pentry->value;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
  table->tombstones_count = 0;
}


