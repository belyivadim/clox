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

  /// Number of tombstones elements in entries array
  i32 tombstones_count;

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

/// Looks up for a value in the tabel associated with the key
///
/// @param table: pointer to the table to look up in
/// @param key: pointer to the key value associated with
/// @outparam value: pointer to the found value (untouched if not found)
/// @return bool, true if value was found, false otherwise
bool table_get(const Table* table, const ObjString *key, Value* value);

/// Deletes entry associated with the key in the table
///
/// @param table: pointer to the table to delete from
/// @param key: pointer to the key value associated with
/// @return bool, true if entry was found and deleted, false otherwise
bool table_delete(Table *table, const ObjString *key);

/// Looks up for a key that are equal to the string in chars array 
///
/// @param table: pointer to the table to look up in
/// @param chars: pointer to char array string object' chars should be equal to
/// @param length: length of the chars array,
/// @param hash: hash of the chars array
/// @return const ObjString*, pointer to the found string object
const ObjString* table_find_string(const Table *table, const char *chars, u32 length, u32 hash);

/// Inserts all entries from src to dest
///
/// @param dest: destination table
/// @param src: source table
/// @return void
void table_add_all(Table* dest, const Table *src);

/// Utilities for GC
void mark_table(Table* table);
void table_remove_white(Table *table);

#endif // !__CLOX_TABLE_H__
