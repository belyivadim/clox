#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"

enum {
  STACK_MAX = 1024
};

/// Represents virtual machine that interprets byte code
typedef struct {
  /// pointer to the Chunk to be interpreted
  Chunk *chunk;

  /// pointer to the current instruction
  u8 *ip;


  /// VM's stack for storing the Values
  Value stack[STACK_MAX];

  /// pointer to the top of the VM's stack,
  /// it points to the slot where next value will be pushed,
  /// and not actual top value of the stack
  Value *stack_top;
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

/// Interprets source code of clox
///
/// @param source pointer to the source code to be interpreted
/// @return InterpreterResult, status code of interpretation
InterpreterResult vm_interpret(const char *source);

/// Pushes a value to the VM's stack
///
/// @param value the Value to be pushed
/// @return void
void vm_stack_push(Value value);

/// Pops the top value from the VM's stack
///
/// @return Value, popped value
Value vm_stack_pop();

#endif // !__CLOX_VM_H__
