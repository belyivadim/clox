#include "common.h"
#include "vm/chunk.h"
#include "vm/debug.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  Chunk chunk;
  chunk_init(&chunk);

  for (i32 i = 0; i < 266; ++i) {
    chunk_write_constant(&chunk, i / 2.0, i + 1);
    printf("%d\n", i);
  }

  chunk_write(&chunk, OP_RETURN, 266);

  
  chunk_write_constant(&chunk, 15555, 267);
  chunk_write(&chunk, OP_RETURN, 267);


  chunk_write(&chunk, OP_RETURN, 268);
  chunk_write(&chunk, OP_RETURN, 268);
  chunk_write(&chunk, OP_RETURN, 268);
  chunk_write(&chunk, OP_RETURN, 300);
  chunk_write(&chunk, OP_RETURN, 300);
  chunk_write(&chunk, OP_RETURN, 300);

  chunk_disassemble(&chunk, "test chunk");

  chunk_free(&chunk);

  return 0;
}
