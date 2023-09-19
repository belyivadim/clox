#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"
#include "object.h"
#include "../utils/table.h"

enum {
  FRAMES_MAX = 64,
  STACK_MAX = FRAMES_MAX * U8_COUNT
};

/// Represents frame of the call stack
typedef struct {
  /// Pointer to the closure being called
  ObjClosure *pclosure;

  /// Instruction pointer of the caller
  ///   VM will jump to this instruction after return
  u8 *ip;

  /// Pointer to the first slot into the VM's value stack
  Value *slots;
} CallFrame;

/// Represents virtual machine that interprets byte code
typedef struct {
  /// Call frame stack
  CallFrame frames[FRAMES_MAX];

  /// Number of active elements in the call frame stack
  i32 frame_count;
  
  /// VM's stack for storing the Values
  Value stack[STACK_MAX];

  /// pointer to the top of the VM's stack,
  /// it points to the slot where next value will be pushed,
  /// and not actual top value of the stack
  Value *stack_top;

  /// Hashtable of global identifiers 
  Table globals;

  /// Hashset of interning strings
  Table strings;

  // Pointer to the head of the list of heap allocated objects
  Obj *objects;
} Vm;

/// Represents status codes of interpretation
typedef enum {
  INTERPRET_OK = 0,
  INTERPRET_COMPILE_ERROR,
  INTERPRETER_RUNTUME_ERROR
} InterpreterResult;

/// Getter for vm singletone
///
/// @return Vm*, pointer to Vm singletone
Vm* vm_instance();

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
