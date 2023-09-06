#ifndef __CLOX_DEBUG_H__

#include "chunk.h"

/// Disassembles instructions in the chunk, and prints it to stdout
///
/// @param chunk the Chunk to be disassembled
/// @param name the name for given @chunk
/// @return void
void chunk_disassemble(Chunk *chunk, const char* name);

/// Disassembles instruction in the chunk located at offset
/// and prints it to stdout
///
/// @param chunk the Chunk where instruction to be disassembled is located
/// @param offset an offset to the instruction
/// @return i32, offset to the next instruction
i32 chunk_disassemble_instruction(Chunk *chunk, i32 offset);

#endif // !__CLOX_DEBUG_H__
