#include "chunk.h"
#include "utils/memory.h"
#include "vm/value.h"

#define GROW_VEC(T, vec, capacity) \
  do { \
      i32 old_capacity = capacity; \
      capacity = GROW_CAPACITY(old_capacity); \
      vec = GROW_ARRAY(T, vec, old_capacity, capacity); \
  } while (0)\

void chunk_init(Chunk *chunk) {
  assert(NULL != chunk);

  chunk->code = NULL;
  chunk->code_count = 0;
  chunk->code_capacity = 0;
  chunk->lines = NULL;
  chunk->lines_count = 0;
  chunk->lines_capacity = 0;

  value_array_init(&chunk->constants);
}

void chunk_free(Chunk *chunk) {
  assert(NULL != chunk);

  FREE_ARRAY(u8, chunk->code, chunk->code_capacity);
  FREE_ARRAY(LineCodeIndexPair, chunk->lines, chunk->lines_capacity);
  value_array_free(&chunk->constants);
  chunk_init(chunk);
}

void chunk_write(Chunk *chunk, u8 byte, i32 line) {
  assert(NULL != chunk);

  if (chunk->code_capacity <= chunk->code_count) {
    GROW_VEC(u8, chunk->code, chunk->code_capacity);
  }

  i32 current_lines_count = chunk->lines_count;

  if (NULL == chunk->lines) {
    GROW_VEC(LineCodeIndexPair, chunk->lines, chunk->lines_capacity);
  }

  // add new line if it is unique
  if (0 == current_lines_count || line != chunk->lines[current_lines_count - 1].line) {
    if (chunk->lines_capacity <= current_lines_count) {
      GROW_VEC(LineCodeIndexPair, chunk->lines, chunk->lines_capacity);
    }

    chunk->lines[current_lines_count].line = line;
    chunk->lines[current_lines_count].code_index = chunk->code_count;

    ++chunk->lines_count;
  }

  chunk->code[chunk->code_count] = byte;
  ++chunk->code_count;
}

void chunk_write_constant(Chunk *chunk, Value value, i32 line) {
  assert(NULL != chunk);

  i32 idx = chunk_add_constant(chunk, value);
  if (idx < 256) {
    chunk_write(chunk, OP_CONSTANT, line);
    chunk_write(chunk, (u8)idx, line);
  } else {
    chunk_write(chunk, OP_CONSTANT_LONG, line);
    chunk_write(chunk, (u8)(idx >> 16), line);
    chunk_write(chunk, (u8)(idx >> 8), line);
    chunk_write(chunk, (u8)(idx), line);
  }
}

i32 chunk_get_constant_long_index(const Chunk *chunk, i32 offset) {
  assert(NULL != chunk);
  assert(offset + 2 < chunk->code_count);

  i32 index = 0;
  index |= (chunk->code[offset] << 16);
  index |= (chunk->code[offset + 1] << 8);
  index |= (chunk->code[offset + 2]);
  return index;
}

i32 chunk_add_constant(Chunk *chunk, Value constant) {
  assert(NULL != chunk);

  value_array_write(&chunk->constants, constant);
  return chunk->constants.count - 1;
}

i32 chunk_get_line(const Chunk *chunk, i32 code_index)
{
  assert(NULL != chunk);

  for (i32 i = 0; i < chunk->lines_count - 1; ++i) {
    if (chunk->lines[i].code_index <= code_index
        && code_index < chunk->lines[i + 1].code_index) {
      return chunk->lines[i].line;
    }
  }

  if (chunk->lines[chunk->lines_count - 1].code_index <= code_index) {
    return chunk->lines[chunk->lines_count - 1].line;
  }

  return -1;
}
