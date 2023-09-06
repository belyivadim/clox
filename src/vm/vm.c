#include <stdio.h>

#include "vm.h"
#include "vm/debug.h"

/// virtual machine singleton
Vm vm;

InterpreterResult vm_run();
void vm_process_constant(Value constant);


void vm_init() {
}

void vm_free() {
}


InterpreterResult vm_interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return vm_run();
}


InterpreterResult vm_run() {
#define OFFSET() (vm.ip - vm.chunk->code)
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
  (vm.chunk->constants.values[\
    chunk_get_constant_long_index(vm.chunk, OFFSET())\
  ])
#define SKIP_BYTES(n) (vm.ip += (n))

#ifdef DEBUG_TRACE_EXECUTION
#define PRINT_DEBUG_INFO() \
  chunk_disassemble_instruction(vm.chunk, OFFSET())
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
      case OP_RETURN:
        return INTERPRET_OK;
    }
  }

#undef PRINT_DEBUG_INFO
#undef SKIP_BYTES
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
#undef OFFSET
}


void vm_process_constant(Value constant) {
  value_print(constant);
  puts("");
}

