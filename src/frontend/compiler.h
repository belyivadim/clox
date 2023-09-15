#ifndef __CLOX_COMPILER_H__
#define __CLOX_COMPILER_H__

#include "../vm/vm.h"
#include "vm/object.h"

/// Compiles the source code of a function to the byte code
///
/// @param source: pointer to the source code
/// @return ObjFunction*, pointer to the compiled function,
///   or NULL if compile time error has been occured
ObjFunction *compile(const char *source);

#endif // !__CLOX_COMPILER_H__
