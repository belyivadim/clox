#ifndef __CLOX_TABLE_H__
#define __CLOX_TABLE_H__

#include "common.h"
#include "../vm/value.h"

/// Represents an entry in a Table
typedef struct {
  /// Key for finding entry in a Table
  const ObjString * key;

  /// Value associated to the key
  Value value;
} Entry;

/// Represents a hash table based on open addresing approach
typedef struct {
  /// Number of active elements in entries array
  i32 count;

  /// Capacity of entries array
  i32 capacity;

  /// Array of entries in hashtable
  Entry *entries;
} Table;


/// Initializes the table to all zeros
///
/// @param table: pointer to the table to be initialized
/// @return void
void table_init(Table *table);

/// Frees underlying array of entries in the table 
/// and initializes all fields in the table to zeros.
///
/// @param table: pointer to the table to be freed
/// @return void
void table_free(Table *table);

/// Inserts new value to the table by key,
/// if key already exists, then overrides old value associated to that key
///
/// @param table: pointer to table to insert value in
/// @param key: pointer to the key to associate value with
/// @param value: value to insert
///
/// @return bool, true if key is new, otherwise false
bool table_set(Table* table, const ObjString *key, Value value);

/// Inserts all entries from src to dest
///
/// @param dest: destination table
/// @param src: source table
/// @return void
void table_add_all(Table* dest, const Table *src);

#endif // !__CLOX_TABLE_H__
