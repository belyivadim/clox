#ifndef __CLOX_CHUNK_H__
#define __CLOX_CHUNK_H__

#include "common.h"
#include "value.h"

/// Represents the operation code
///   all *_LONG versions of opcode have numeric value  +1 to their non-long versions
typedef enum {
  /// Takes a single byte operand
  ///   that specifies the index in Chunk's constants array to the constant value
  ///
  /// @Interpreting: Pushes constant onto VM's stack.
  /// 
  /// @size - 2 bytes
  OP_CONSTANT, 

  /// Takes 3 bytes (24-bits) operand
  /// that specifies the index in Chunk's constants array to the constant value
  ///
  /// opcode essentially is the same as OP_CONSTANT, but allowed indexes greater than 255
  ///
  /// @Interpreting: Pushes constant onto VM's stack.
  ///
  /// @size - 4 bytes
  OP_CONSTANT_LONG,

  /// Bare opcode, takes no operands
  /// @Interpreting: Represents a nil value
  /// @size - 1 byte
  OP_NIL,

  /// Bare opcode, takes no operands
  /// @Interpreting: Represents a boolean true value
  /// @size - 1 byte
  OP_TRUE,

  /// Bare opcode, takes no operands
  /// @Interpreting: Represents a boolean false value
  /// @size - 1 byte
  OP_FALSE,


  /// Bare opcodes, take no operands
  /// @Interpreting: Pops two values from the top of the VM's stack,
  ///   perform comparison, and push result back onto the stack as bool
  /// @size - 1 byte
  OP_EQUAL,
  OP_NOT_EQUAL,
  OP_GREATER,
  OP_GREATER_EQUAL,
  OP_LESS,
  OP_LESS_EQUAL,

  /// Bare opcode, takes no operands
  /// @Interpreting: Converts a value to boolean and applies not to it
  /// @size - 1 byte
  OP_NOT,

  /// Bare opcode, takes no operands
  /// @Interpreting: Negates top value of the VM's stack
  /// @size - 1 byte
  OP_NEGATE,

  /// Bare opcode, takes no operands
  /// @Interpreting: Pops two values from the top of the VM's stack,
  ///   adds them, and pushes result back onto the stack
  /// @size - 1 byte
  OP_ADD,

  /// Bare opcode, takes no operands
  /// @Interpreting: Pops two values from the top of the VM's stack,
  ///   substracts them, and pushes result back onto the stack
  /// @size - 1 byte
  OP_SUBSTRACT,

  /// Bare opcode, takes no operands
  /// @Interpreting: Pops two values from the top of the VM's stack,
  ///   multiplies them, and pushes result back onto the stack
  /// @size - 1 byte
  OP_MULTIPLY,

  /// Bare opcode, takes no operands
  /// @Interpreting: Pops two values from the top of the VM's stack,
  /// divides them, and pushes result back onto the stack
  /// @size - 1 byte
  OP_DIVIDE,

  /// Bare opcode, takes no operands
  /// Used by built-in print statements
  /// @Interpreting: prints to stdout 
  ///   string representation of the top value on the stack,
  ///   pops top value from the stack
  /// @size - 1 byte
  OP_PRINT,

  /// Bare opcode, takes no operands
  /// @Interpreting: simply pops top value from the stack
  /// @size - 1 byte
  OP_POP,

  /// Takes 1 byte operand that is index in chunk's constants array
  ///   which is pointed to the global variable name
  ///
  /// @Interpreting: reads the operand, gets the name from chunk's constants array
  ///   and defines a global variable with that name, assigning the top value from VM's stack to it
  /// @size - 2 bytes
  OP_DEFINE_GLOBAL,

  /// Takes 3 bytes (24-bits) operand
  ///   which is pointed to the global variable name
  ///
  /// @essentially is the same as OP_DEFINE_GLOBAL, difference is only in operand size
  /// @size - 4 bytes
  OP_DEFINE_GLOBAL_LONG,


  /// Takes 1 byte operand that is index in chunk's constants array
  ///   which is pointed to the global variable name
  ///
  /// @Interpreting: reads operand, looks up for that name in VM's globals table,
  ///   if such key is present pushes associated value to the VM's stack,
  ///   otherwise reports runtime error and returns with INTERPRET_RUNTIME_ERROR
  /// @size - 2 bytes
  OP_GET_GLOBAL,

  /// Takes 3 bytes (24-bits) operand
  ///   which is pointed to the global variable name
  ///
  /// @essentially is the same as OP_GET_GLOBAL, difference is only in operand size
  /// @size - 4 bytes
  OP_GET_GLOBAL_LONG,

  /// Takes 1 byte operand that is index in chunk's constants array
  ///   which is pointed to the global variable name
  ///
  /// @Interpreting: reads operand, looks up for that name in VM's globals table,
  ///   if such key is present pushes overrites it with value from the top of VM's stack,
  ///   otherwise reports runtime error and returns with INTERPRET_RUNTIME_ERROR
  /// @size - 2 bytes
  OP_SET_GLOBAL,
  
  /// Takes 3 bytes (24-bits) operand
  ///   which is pointed to the global variable name
  ///
  /// @essentially is the same as OP_SET_GLOBAL, difference is only in operand size
  /// @size - 4 bytes
  OP_SET_GLOBAL_LONG,

  /// Takes 1 byte operand that is index in VM's stack
  ///   which is pointed to the local variable value
  ///
  /// @Interpreting: reads operand, assigns VM's top stack value to VM's stack value at operand 
  /// @size - 2 byte
  OP_GET_LOCAL,

  /// Takes 1 byte operand that is index in VM's stack
  ///   which is pointed to the local variable value
  ///
  /// @Interpreting: reads operand, pushes value at that operand on VM's stack onto VM's stack
  /// @size - 2 byte
  OP_SET_LOCAL,

  /// @size - 2 byte
  OP_GET_UPVALUE,

  /// @size - 2 byte
  OP_SET_UPVALUE,

  /// Takes 2 (16-bits) byte operand that is offset in bytes how many bytes to jump
  ///
  /// @Interpreting: reads the operand, then increments VM's ip on the read value
  /// @size - 3 bytes
  OP_JUMP,

  /// Takes 2 (16-bits) byte operand that is offset in bytes how many bytes to jump
  ///
  /// @Interpreting: reads the operand, then increments VM's ip on the read value
  ///   in case if value on top of the VM's stack is falsey
  /// @size - 3 bytes
  OP_JUMP_IF_FALSE,

  /// Takes 2 (16-bits) byte operand that is offset in bytes 
  ///   how many bytes to jump backwards to the beginning of the loop
  ///
  /// @Interpreting: reads the operand, then decrements VM's ip on the read value
  /// @size - 3 bytes
  OP_LOOP,

  /// Takes 1 byte operand that is number of arguments being passed
  ///
  /// @Interpreting: reads the operand as argc, 
  ///   then call value on top of the stack with argc
  /// @size = 2 bytes
  OP_CALL,

  /// Takes 1 byte operand that is index of the compiled function 
  ///   in current chunk's constants array
  ///
  /// @Interpreting:
  /// @size = 2 bytes
  OP_CLOSURE,

  /// Bare opcode, takes no operands
  /// @size - 1 byte
  OP_RETURN, 
} OpCode;

/// Represents pair of the line number in the source code
/// and the code index of an opcode in code array
typedef struct {
  i32 line;
  i32 code_index;
} LineCodeIndexPair;


/// Represents the Chunk of op codes
typedef struct {
  /// pointer to the dynamic array of op codes
  u8 *code;

  /// number of active element in the code array
  i32 code_count;

  /// capacity of the code array
  i32 code_capacity;

  /// pointer to the dynamic array of a pairs of the line numbers and code indexes,
  /// corresponding to where current op code is located in source file;
  LineCodeIndexPair *lines;

  /// number of active elements in the lines array
  i32 lines_count;

  /// capacity of the lines array
  i32 lines_capacity;

  /// array of constant values
  ValueArray constants;
} Chunk;


/// Initializes all fields of the chunk to zeros
///
/// @param chunk pointer to the Chunk to be initialized
/// @return void
void chunk_init(Chunk *chunk);

/// Frees memory allocated for the chunk and clean up its fields (set to zeros)
///
/// @param chunk pointer to the Chunk to be freed
/// @return void
void chunk_free(Chunk *chunk);

/// Writes a byte to the next avialable byte in the chunk
///
/// @param chunk pointer to the Chunk to be written to
/// @param byte a byte to be written to the chunk
/// @param line a number of line in the source file where entity represented by byte is located
/// @return void
void chunk_write(Chunk *chunk, u8 byte, i32 line);

/// Writes a constant value to the Chunk's constants array.
/// It will write it as OP_CONSTANT if the index of that constant will be less than 256
/// or as OP_CONSTANT_LONG otherwise
///
/// @param chunk pointer to the Chunk to be written to
/// @param value value to be written
/// @param line a number of line in the source file where entity represented by byte is located
/// @return i32, the index in chunk's constant array, where the constant was appended
i32 chunk_write_constant(Chunk *chunk, Value value, i32 line);

/// Reads 3 bytes from the Chunk's code array starting from the offset and converts it to the index i32.
///
/// @param chunk pointer to the Chunk to read from
/// @param offset offset to the first byte of index
/// @return i32, the index of constant value in Chunk's constants array
i32 chunk_get_constant_long_index(const Chunk *chunk, i32 offset);

/// Adds constant to the chunk's constants array
///
/// @param chunk pointer to the Chunk to be written to
/// @param constant constant value to be written to the chunk's constants array
/// @return i32, the index where the constant was appended
i32 chunk_add_constant(Chunk *chunk, Value constant);


/// Searching a number of the line in source code for opcode at code_index
///
/// @param chunk pointer to the Chunk where opcode is located
/// @code_index index of the OpCode whose number of the line in source code should be found
/// @return i32, a number of the line in the source code, or -1 if not found
i32 chunk_get_line(const Chunk *chunk, i32 code_index);

#endif // !__CLOX_CHUNK_H__
