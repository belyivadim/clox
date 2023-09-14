#include <stdarg.h>
#include <stdio.h>
#include <memory.h>

#include "vm.h"
#include "vm/debug.h"
#include "../frontend/compiler.h"
#include "../utils/memory.h"
#include "object.h"

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

/// Checks if passed value is falsey.
/// if value is nil, false or number 0, it is falsey,
///
/// @param value: value to be checked
/// return bool
static bool is_falsey(Value value);

/// Reports runtime error to stderr with formated message
static void runtime_error(const char *format, ...);

/// Concatenates two strings on top of the stack,
/// pops string that are to be concatenated from the stack,
/// pushes new concatenated string onto the stack
///
/// @return void
static void concatenate();

static InterpreterResult vm_run();
static void vm_process_constant(Value constant);
static void vm_process_define_global(const ObjString *name);
static InterpreterResult vm_process_get_global(const ObjString *name);
static InterpreterResult vm_process_set_global(const ObjString *name);
static void vm_process_return();

Vm* vm_instance() {
  static Vm vm;
  return &vm;
}

void vm_init() {
  vm_stack_reset();
  Vm* vm = vm_instance();
  vm->objects = NULL;
  table_init(&vm->globals);
  table_init(&vm->strings);
}

void vm_free() {
  free_objects();
  Vm* vm = vm_instance();
  table_free(&vm->globals);
  table_free(&vm->strings);
}

void vm_stack_push(Value value) {
  Vm *vm = vm_instance();
  assert(vm->stack_top - vm->stack < STACK_MAX);

  *vm->stack_top = value;
  ++vm->stack_top;
}

Value vm_stack_pop() {
  Vm *vm = vm_instance();
  assert(vm->stack_top > vm->stack);

  --vm->stack_top;
  return *vm->stack_top;
}

static const Value* vm_stack_peek(i32 distance) {
  Vm *vm = vm_instance();
  assert(vm->stack_top - 1 - distance >= vm->stack);
  return vm->stack_top - 1 - distance;
}

static Value* vm_stack_top() {
  Vm *vm = vm_instance();
  assert(vm->stack_top - 1 >= vm->stack);
  return vm->stack_top - 1;
}

static void vm_stack_reset() {
  Vm *vm = vm_instance();
  vm->stack_top = vm->stack;
}

static bool is_falsey(Value value) {
  return IS_NIL(value) 
      || (IS_BOOL(value) && !AS_BOOL(value))
      || (IS_NUMBER(value) && (AS_NUMBER(value)) == 0);
}

static void concatenate() {
  ObjString *rhs = AS_STRING(vm_stack_pop());
  ObjString *lhs = AS_STRING(vm_stack_pop());

  u32 length = lhs->length + rhs->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, lhs->chars, lhs->length);
  memcpy(chars + lhs->length, rhs->chars, rhs->length);
  chars[length] = '\0';

  const ObjString * result = string_create(chars, length);
  vm_stack_push(OBJ_VAL(result));
}

InterpreterResult vm_interpret(const char *source) {
  assert(NULL != source);

  Chunk chunk;
  chunk_init(&chunk);

  if (!compile(source, &chunk)) {
    chunk_free(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  Vm *vm = vm_instance();
  vm->chunk = &chunk;
  vm->ip = vm->chunk->code;

  InterpreterResult result = vm_run();

  chunk_free(&chunk);

  return result;
}


static InterpreterResult vm_run() {
#define OFFSET() (vm->ip - vm->chunk->code)
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
  (vm->chunk->constants.values[\
    chunk_get_constant_long_index(vm->chunk, OFFSET())\
  ])
#define SKIP_BYTES(n) (vm->ip += (n))

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())

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
    for (Value *slot = vm->stack; slot < vm->stack_top; ++slot) { \
      printf("( "); \
      value_print(*slot); \
      printf(" )"); \
    } \
    puts(""); \
    chunk_disassemble_instruction(vm->chunk, OFFSET()); \
  } while (0)
#else
#define PRINT_DEBUG_INFO() (void)0
#endif /* !DEBUG_TRACE_EXECUTION */

  // function body starts here
  Vm *vm = vm_instance();
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

      case OP_ADD: {
        if (IS_STRING(*vm_stack_peek(0)) && IS_STRING(*vm_stack_peek(1))) {
          concatenate();
        } else if (IS_NUMBER(*vm_stack_peek(0)) && IS_NUMBER(*vm_stack_peek(1))) {
          double rhs = AS_NUMBER(vm_stack_pop()); 
          Value *plhs = vm_stack_top(); 
          *plhs = NUMBER_VAL(AS_NUMBER(*plhs) + rhs); 
        } else {
          runtime_error("Operands must be two numbers or two strings.");
          return INTERPRETER_RUNTUME_ERROR;
        }
        break;
      }
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

      case OP_PRINT: {
        value_print(vm_stack_pop());
        printf("\n");
        break;
      }

      case OP_POP: vm_stack_pop(); break;

      case OP_DEFINE_GLOBAL: {
        vm_process_define_global(READ_STRING());
        break;
      }

      case OP_DEFINE_GLOBAL_LONG: {
        vm_process_define_global(READ_STRING_LONG());
        SKIP_BYTES(3);
        break;
      }

      case OP_GET_GLOBAL: {
        InterpreterResult result = vm_process_get_global(READ_STRING());
        if (INTERPRET_OK != result) {
          return result;
        }
        break;
      }

      case OP_GET_GLOBAL_LONG: {
        InterpreterResult result = vm_process_get_global(READ_STRING_LONG());
        if (INTERPRET_OK != result) {
          return result;
        }
        SKIP_BYTES(3);
        break;
      }

      case OP_SET_GLOBAL: {
        InterpreterResult result = vm_process_set_global(READ_STRING());
        if (INTERPRET_OK != result) {
          return result;
        }
        break;
      }

      case OP_SET_GLOBAL_LONG: {
        InterpreterResult result = vm_process_set_global(READ_STRING_LONG());
        if (INTERPRET_OK != result) {
          return result;
        }
        SKIP_BYTES(3);
        break;
      }

      case OP_GET_LOCAL: {
        u8 slot = READ_BYTE();
        vm_stack_push(vm->stack[slot]);
        break;
      }

      case OP_SET_LOCAL: {
        u8 slot = READ_BYTE();
        vm->stack[slot] = *vm_stack_top();
        break;
      }

      case OP_RETURN:
        return INTERPRET_OK;
    }
  }

#undef PRINT_DEBUG_INFO
#undef BINARY_OP
#undef SKIP_BYTES
#undef READ_STRING
#undef READ_STRING_LONG
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
#undef OFFSET
}


static void vm_process_constant(Value constant) {
  vm_stack_push(constant);
}

static void vm_process_define_global(const ObjString *name) {
  table_set(&vm_instance()->globals, name, *vm_stack_peek(0));
  vm_stack_pop();
}

static InterpreterResult vm_process_get_global(const ObjString *name) {
  Value value;
  if (!table_get(&vm_instance()->globals, name, &value)) {
    runtime_error("Undefined variable '%s'.", name->chars);
    return INTERPRETER_RUNTUME_ERROR;
  }

  vm_stack_push(value);
  return INTERPRET_OK;
}

static InterpreterResult vm_process_set_global(const ObjString *name) {
  Table *globals = &vm_instance()->globals;
  if (table_set(globals, name, *vm_stack_top())) {
    table_delete(globals, name);
    runtime_error("Undefined variable '%s'.", name->chars);
    return INTERPRETER_RUNTUME_ERROR;
  }
  return INTERPRET_OK;
}

static void vm_process_return() {
  value_print(vm_stack_pop());
  puts("");
}


static void runtime_error(const char *format, ...) {
#define ERROR_COLOR COLOR_FG_RED

  Vm *vm = vm_instance();
  i32 instruction = vm->ip - vm->chunk->code - 1;
  i32 line = chunk_get_line(vm->chunk, instruction);
  fprintf(stderr, ERROR_COLOR "[line %d] in script: ", line);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n" COLOR_FG_RESET, stderr);
  
  vm_stack_reset();

#undef ERROR_COLOR
}
