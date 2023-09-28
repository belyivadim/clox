#include <stdio.h>

#include "debug.h"
#include "vm/object.h"

static i32 simple_instruction(const char *name, i32 offset);
static i32 constant_instruction(const char *name, const Chunk *chunk, i32 offset);
static i32 constant_long_instruction(const char *name, const Chunk *chunk, i32 offset);
static i32 byte_instruction(const char *name, const Chunk *chunk, i32 offset);
static i32 jump_instruction(const char *name, i32 sign, const Chunk *chunk, i32 offset);
static i32 invoke_instruction(const char *name, const Chunk* chunk, i32 offset);
static i32 invoke_long_instruction(const char *name, const Chunk* chunk, i32 offset);

#define DISASSEMBLE_COLOR COLOR_FG_CYAN

void chunk_disassemble(Chunk *chunk, const char* name) {
  assert(NULL != chunk);

  printf(DISASSEMBLE_COLOR "== %s ==\n", name);

  for (i32 offset = 0; offset < chunk->code_count;) {
    offset = chunk_disassemble_instruction(chunk, offset);
  }
  printf(COLOR_FG_RESET);
}

i32 chunk_disassemble_instruction(Chunk *chunk, i32 offset) {
  assert(NULL != chunk);
  assert(NULL != chunk->code);
  assert(NULL != chunk->lines);
  assert(offset < chunk->code_count);

  printf(DISASSEMBLE_COLOR "%04d ", offset);

  i32 curr_line = chunk_get_line(chunk, offset);
  i32 prev_line = chunk_get_line(chunk, offset - 1);
  if (offset > 0 
      && curr_line == prev_line) {
    printf("   | " COLOR_FG_RESET);
  } else {
    printf("%4d " COLOR_FG_RESET, curr_line);
  }

  u8 instruction = chunk->code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
      return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
    case OP_NIL:
      return simple_instruction("OP_NIL", offset);
    case OP_TRUE:
      return simple_instruction("OP_TRUE", offset);
    case OP_FALSE:
      return simple_instruction("OP_FALSE", offset);
    case OP_NOT:
      return simple_instruction("OP_NOT", offset);
    case OP_EQUAL:
      return simple_instruction("OP_EQUAL", offset);
    case OP_NOT_EQUAL:
      return simple_instruction("OP_NOT_EQUAL", offset);
    case OP_GREATER:
      return simple_instruction("OP_GREATER", offset);
    case OP_GREATER_EQUAL:
      return simple_instruction("OP_GREATER_EQUAL", offset);
    case OP_LESS:
      return simple_instruction("OP_LESS", offset);
    case OP_LESS_EQUAL:
      return simple_instruction("OP_LESS_EQUAL", offset);
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_SUBSTRACT:
      return simple_instruction("OP_SUBSTRACT", offset);
    case OP_MULTIPLY:
      return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_PRINT:
      return simple_instruction("OP_PRINT", offset);
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    case OP_POP:
      return simple_instruction("OP_POP", offset);
    case OP_DEFINE_GLOBAL:
      return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL_LONG:
      return constant_long_instruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
    case OP_GET_GLOBAL:
      return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL_LONG:
      return constant_long_instruction("OP_GET_GLOBAL_LONG", chunk, offset);
    case OP_SET_GLOBAL:
      return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL_LONG:
      return constant_long_instruction("OP_SET_GLOBAL_LONG", chunk, offset);
    case OP_GET_LOCAL:
      return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byte_instruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_UPVALUE:
      return byte_instruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byte_instruction("OP_SET_UPVALUE", chunk, offset);
    case OP_JUMP:
      return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
      return jump_instruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
      return byte_instruction("OP_CALL", chunk, offset);
    case OP_INVOKE:
      return invoke_instruction("OP_INVOKE", chunk, offset);
    case OP_INVOKE_LONG:
      return invoke_long_instruction("OP_INVOKE_LONG", chunk, offset);
    case OP_CLOSURE: {
      ++offset;
      u8 constant = chunk->code[offset++];
      printf(DISASSEMBLE_COLOR "%-16s [%6d] ", "OP_CLOSURE", constant);
      value_print(chunk->constants.values[constant]);
      puts("");

      ObjFunction * pfun = AS_FUNCTION(chunk->constants.values[constant]);
      for (i32 j = 0; j < pfun->upvalue_count; ++j) {
        i32 is_local = chunk->code[offset++];
        i32 index = chunk->code[offset++];
        printf("%04d      |                      %s %d\n",
               offset - 2, is_local ? "local" : "upvalue", index);
      }

      printf(COLOR_FG_RESET);

      return offset;
    }

    case OP_CLOSE_UPVALUE:
      return simple_instruction("OP_CLOSE_UPVALUE", offset);

    case OP_CLASS:
      return constant_instruction("OP_CLASS", chunk, offset);
    case OP_CLASS_LONG:
      return constant_long_instruction("OP_CLASS_LONG", chunk, offset);

    case OP_GET_PROPERTY:
      return constant_instruction("OP_GET_PROPERTY", chunk, offset);
    case OP_GET_PROPERTY_LONG:
      return constant_long_instruction("OP_GET_PROPERTY_LONG", chunk, offset);
    case OP_SET_PROPERTY:
      return constant_instruction("OP_SET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY_LONG:
      return constant_long_instruction("OP_SET_PROPERTY_LONG", chunk, offset);

    case OP_METHOD:
      return constant_instruction("OP_METHOD", chunk, offset);
    case OP_METHOD_LONG:
      return constant_long_instruction("OP_METHOD_LONG", chunk, offset);

    default:
      printf(DISASSEMBLE_COLOR "Unknown opcode %d\n" COLOR_FG_RESET, instruction);
      return offset + 1;
  }

  printf(COLOR_FG_RESET);
}

static i32 simple_instruction(const char *name, i32 offset) {
  printf(DISASSEMBLE_COLOR "%s\n" COLOR_FG_RESET, name);
  return offset + 1;
}

static i32 constant_instruction(const char *name, const Chunk *chunk, i32 offset) {
  assert(NULL != chunk);
  assert(NULL != chunk->constants.values);

  u8 idx = chunk->code[offset + 1];
  printf(DISASSEMBLE_COLOR "%-16s [%6d] '", name, idx);
  value_print(chunk->constants.values[idx]);
  puts("'" COLOR_FG_RESET);

  return offset + 2;
}

static i32 constant_long_instruction(const char *name, const Chunk *chunk, i32 offset) {
  assert(NULL != chunk);
  assert(NULL != chunk->constants.values);

  i32 idx = chunk_get_constant_long_index(chunk, offset + 1);
  printf(DISASSEMBLE_COLOR "%-16s [%6d] '", name, idx);
  value_print(chunk->constants.values[idx]);
  puts("'" COLOR_FG_RESET);

  return offset + 4;
}


static i32 byte_instruction(const char *name, const Chunk *chunk, i32 offset) {
  u8 slot = chunk->code[offset + 1];
  printf(DISASSEMBLE_COLOR "%-16s [%6d]\n" COLOR_FG_RESET, name, slot);
  return offset + 2;
}

static i32 jump_instruction(const char *name, i32 sign, const Chunk *chunk, i32 offset) {
  u16 jump = (u16)((chunk->code[offset + 1] << 8) | chunk->code[offset + 2]);
  printf(DISASSEMBLE_COLOR "%-16s [%6d] -> %d\n" COLOR_FG_RESET,
          name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

static i32 invoke_instruction(const char *name, const Chunk* chunk, i32 offset) {
  u8 constant = chunk->code[offset + 1];
  u8 argc = chunk->code[offset + 2];
  printf(DISASSEMBLE_COLOR "%-16s (%d argc) [%6d] ", name, argc, constant);
  value_print(chunk->constants.values[constant]);
  puts(COLOR_FG_RESET);
  return offset + 3;
}

static i32 invoke_long_instruction(const char *name, const Chunk* chunk, i32 offset) {
  i32 constant = chunk_get_constant_long_index(chunk, offset + 1);
  u8 argc = chunk->code[offset + 4];
  printf(DISASSEMBLE_COLOR "%-16s (%d argc) [%6d] " COLOR_FG_RESET, name, argc, constant);
  value_print(chunk->constants.values[constant]);
  puts(COLOR_FG_RESET);
  return offset + 5;
}

