#include <stdio.h>

#include "vm.h"
#include "vm/debug.h"

/// virtual machine singleton
Vm vm;

static void vm_stack_reset();
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


static void vm_stack_reset() {
  vm.stack_top = vm.stack;
}

InterpreterResult vm_interpret(Chunk *chunk) {
  assert(NULL != chunk);

  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return vm_run();
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

#define BINARY_OP(op) \
  do { \
    double b = vm_stack_pop(); \
    double a = vm_stack_pop(); \
    vm_stack_push(a op b); \
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

      case OP_ADD:        BINARY_OP(+); break;
      case OP_SUBSTRACT:  BINARY_OP(-); break;
      case OP_MULTIPLY:   BINARY_OP(*); break;
      case OP_DIVIDE:     BINARY_OP(/); break;

      case OP_NEGATE: {
        *(vm.stack_top - 1) = -*(vm.stack_top - 1);
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
