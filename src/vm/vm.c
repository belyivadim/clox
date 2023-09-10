#include <stdarg.h>
#include <stdio.h>

#include "vm.h"
#include "vm/debug.h"
#include "../frontend/compiler.h"

/// virtual machine singleton
Vm vm;

static void vm_stack_reset();

/// Peeks the value on the stack at the position number_of_elements - distance 
///
/// @param distance: distance to the element from the top of the stack
/// @return const Value*, constant pointer to peeked Value
static const Value* vm_stack_peek(i32 distance);

/// Peeks the value on the top of the stack 
///
/// @return Value*, pointer to peeked Value
static Value* vm_stack_top();

static bool is_falsey(Value value);

/// Reports runtime error to stderr with formated message
static void runtime_error(const char *format, ...);

static InterpreterResult vm_run();
static void vm_process_constant(Value constant);
static void vm_process_return();


void vm_init() {
  vm_stack_reset();
}

void vm_free() {
}

void vm_stack_push(Value value) {
  assert(vm.stack_top - vm.stack < STACK_MAX);

  *vm.stack_top = value;
  ++vm.stack_top;
}

Value vm_stack_pop() {
  assert(vm.stack_top > vm.stack);

  --vm.stack_top;
  return *vm.stack_top;
}

static const Value* vm_stack_peek(i32 distance) {
  assert(vm.stack_top - 1 - distance >= vm.stack);
  return vm.stack_top - 1 - distance;
}

static Value* vm_stack_top() {
  assert(vm.stack_top - 1 >= vm.stack);
  return vm.stack_top - 1;
}

static void vm_stack_reset() {
  vm.stack_top = vm.stack;
}

static bool is_falsey(Value value) {
  return IS_NIL(value) 
      || (IS_BOOL(value) && !AS_BOOL(value))
      || (IS_NUMBER(value) && (AS_NUMBER(value)) == 0);
}

InterpreterResult vm_interpret(const char *source) {
  assert(NULL != source);

  Chunk chunk;
  chunk_init(&chunk);

  if (!compile(source, &chunk)) {
    chunk_free(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpreterResult result = vm_run();

  chunk_free(&chunk);

  return result;
}


static InterpreterResult vm_run() {
#define OFFSET() (vm.ip - vm.chunk->code)
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
  (vm.chunk->constants.values[\
    chunk_get_constant_long_index(vm.chunk, OFFSET())\
  ])
#define SKIP_BYTES(n) (vm.ip += (n))

#define BINARY_OP(value_kind, op) \
  do { \
    if (!IS_NUMBER(*vm_stack_peek(0)) || !IS_NUMBER(*vm_stack_peek(1))) {\
      runtime_error("Operands must be a numbers.");\
      return INTERPRETER_RUNTUME_ERROR;\
    } \
    double rhs = AS_NUMBER(vm_stack_pop()); \
    Value *plhs = vm_stack_top(); \
    *plhs = value_kind(AS_NUMBER(*plhs) op rhs); \
  } while (0)

#ifdef DEBUG_TRACE_EXECUTION
#define PRINT_DEBUG_INFO() \
  do { \
    printf("          "); \
    for (Value *slot = vm.stack; slot < vm.stack_top; ++slot) { \
      printf("( "); \
      value_print(*slot); \
      printf(" )"); \
    } \
    puts(""); \
    chunk_disassemble_instruction(vm.chunk, OFFSET()); \
  } while (0)
#else
#define PRINT_DEBUG_INFO() (void)0
#endif /* !DEBUG_TRACE_EXECUTION */

  // function body starts here
  for (;;) {
    PRINT_DEBUG_INFO();
    u8 instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        vm_process_constant(READ_CONSTANT());
        break;
      }
      case OP_CONSTANT_LONG: {
        vm_process_constant(READ_CONSTANT_LONG());
        SKIP_BYTES(3);
        break;
      }

      case OP_NIL: vm_stack_push(NIL_VAL); break;
      case OP_TRUE: vm_stack_push(BOOL_VAL(true)); break;
      case OP_FALSE: vm_stack_push(BOOL_VAL(false)); break;

      case OP_ADD:        BINARY_OP(NUMBER_VAL, +); break;
      case OP_SUBSTRACT:  BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
      
      case OP_NOT:
        *vm_stack_top() = BOOL_VAL(is_falsey(*vm_stack_top()));
        break;

      case OP_EQUAL: {
        Value rhs = vm_stack_pop();
        *vm_stack_top() = BOOL_VAL(values_equal(*vm_stack_top(), rhs));
        break;
      }

      case OP_NOT_EQUAL: {
        Value rhs = vm_stack_pop();
        *vm_stack_top() = BOOL_VAL(!values_equal(*vm_stack_top(), rhs));
        break;
      }

      case OP_GREATER:       BINARY_OP(BOOL_VAL, >); break;
      case OP_GREATER_EQUAL: BINARY_OP(BOOL_VAL, >=); break;
      case OP_LESS:          BINARY_OP(BOOL_VAL, <); break;
      case OP_LESS_EQUAL:    BINARY_OP(BOOL_VAL, <=); break;

      case OP_NEGATE: {
        if (!(IS_NUMBER(*vm_stack_peek(0)))) {
          runtime_error("Operand must be a number.");
          return INTERPRETER_RUNTUME_ERROR;
        }
        vm_stack_top()->as.number = -AS_NUMBER(*vm_stack_top());
        break;
      }
      case OP_RETURN:
        vm_process_return();
        return INTERPRET_OK;
    }
  }

#undef PRINT_DEBUG_INFO
#undef BINARY_OP
#undef SKIP_BYTES
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
#undef OFFSET
}


static void vm_process_constant(Value constant) {
  vm_stack_push(constant);
}

static void vm_process_return() {
  value_print(vm_stack_pop());
  puts("");
}


static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);
  
  i32 instruction = vm.ip - vm.chunk->code - 1;
  i32 line = chunk_get_line(vm.chunk, instruction);

  fprintf(stderr, "[line %d] in script\n", line);

  vm_stack_reset();
}
