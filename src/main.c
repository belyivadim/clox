#include "common.h"
#include "vm/chunk.h"
#include "vm/debug.h"
#include "vm/vm.h"


int main(int argc, char *argv[])
{
  vm_init();

  Chunk chunk;
  chunk_init(&chunk);

  chunk_write_constant(&chunk, 1.2, 1);
  chunk_write_constant(&chunk, 3.4, 1);

  chunk_write(&chunk, OP_ADD, 1);

  chunk_write_constant(&chunk, 2, 1);

  chunk_write(&chunk, OP_DIVIDE, 1);
  chunk_write(&chunk, OP_NEGATE, 1);

  chunk_write(&chunk, OP_RETURN, 2);
  
  // chunk_disassemble(&chunk, "test chunk");
  vm_interpret(&chunk);

  vm_free();
  chunk_free(&chunk);

  return 0;
}
