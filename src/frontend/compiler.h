#ifndef __CLOX_COMPILER_H__
#define __CLOX_COMPILER_H__

#include "../vm/vm.h"

/// Compiles the source code to the byte code
/// and puts that byte code into the chunk
///
/// @param source: pointer to the source code
/// @out-param chunk: pointer to the chunk byte code will be put into
/// @return bool, true on successful compilation, otherwise false
bool compile(const char *source, Chunk *chunk);

#endif // !__CLOX_COMPILER_H__
