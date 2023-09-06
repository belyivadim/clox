#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"

/// Represents virtual machine that interprets byte code
typedef struct {
  /// pointer to the Chunk to be interpreted
  Chunk *chunk;

  /// pointer to the current instruction
  u8 *ip;
} Vm;

/// Represents status codes of interpretation
typedef enum {
  INTERPRET_OK = 0,
  INTERPRET_COMPILE_ERROR,
  INTERPRETER_RUNTUME_ERROR
} InterpreterResult;

/// Initializes virtual machine (singletone)
///
/// @return void
void vm_init();

/// Frees virtual machine (singletone)
///
/// @return void
void vm_free();

/// Interprets chunk of the byte code
///
/// @param chunk pointer to the Chunk to be interpreted
/// @return InterpreterResult, status code of interpretation
InterpreterResult vm_interpret(Chunk *chunk);


#endif // !__CLOX_VM_H__
