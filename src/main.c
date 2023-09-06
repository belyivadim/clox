#include "common.h"
#include "vm/chunk.h"
#include "vm/debug.h"
#include "vm/vm.h"


int main(int argc, char *argv[])
{
  vm_init();

  Chunk chunk;
  chunk_init(&chunk);

  for (i32 i = 0; i < 266; ++i) {
    chunk_write_constant(&chunk, i / 2.0, i + 1);
  }

  chunk_write(&chunk, OP_RETURN, 266);

  chunk_disassemble(&chunk, "test chunk");
  vm_interpret(&chunk);

  vm_free();
  chunk_free(&chunk);

  return 0;
}
