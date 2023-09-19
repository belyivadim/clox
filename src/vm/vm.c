#include <stdarg.h>
#include <stdio.h>
#include <memory.h>

#include "vm.h"
#include "vm/debug.h"
#include "../frontend/compiler.h"
#include "../utils/memory.h"
#include "object.h"

#include "native/native_time.h"
#include "native/native_io.h"

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

/// Calls the callee by allocating new frame onto the VM's frames
/// 
/// @param callee: callable Value
/// @param arg_count: number of arguments being passed
/// @return bool, true if call has processed successfully,
///   and number of argumets is the same as arity
///   false otherwise
static bool vm_call_value(Value callee, i32 arg_count);

/// Calls closure's function by allocating new frame onto the VM's frames
///
/// @param pclosure: pointer to the closure which function should be called
/// @param arg_count: number of arguments being passed
/// @return bool, true if number of argumets is the same as arity,
///   false otherwise
static bool vm_call(ObjClosure *pclosure, i32 arg_count);

/// Defines native function in the global namespace
///
/// @param name: pointer to the c-string that represents 
///   native function's name
/// @param pfun: pointer to the function that will be called
///   by calling native function
/// @param arity: number of arguments pfun expects
/// @return void
static void native_define(const char *name, NativeFn pfun, i32 arity);

static InterpreterResult vm_run();
static void vm_process_constant(Value constant);
static void vm_process_define_global(const ObjString *name);
static InterpreterResult vm_process_get_global(const ObjString *name);
static InterpreterResult vm_process_set_global(const ObjString *name);
static void vm_process_return();

static ObjUpvalue *capture_upvalue(Value *local);

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

  native_define("clock", clock_native, 0);
  native_define("readln", readln_native, 0);
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
  vm->frame_count = 0;
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

  ObjFunction *pfun = compile(source);
  if (NULL == pfun) return INTERPRET_COMPILE_ERROR;

  vm_stack_push(OBJ_VAL(pfun));
  ObjClosure *pclosure = closure_create(pfun);
  vm_stack_pop();
  vm_stack_push(OBJ_VAL(pclosure));
  vm_call_value(OBJ_VAL(pclosure), 0);
  
  return vm_run();
}


static InterpreterResult vm_run() {
#define OFFSET() (frame->ip - frame->pclosure->pfun->chunk.code)
#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->pclosure->pfun->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
  (frame->pclosure->pfun->chunk.constants.values[\
    chunk_get_constant_long_index(&frame->pclosure->pfun->chunk, OFFSET())\
  ])
#define SKIP_BYTES(n) (frame->ip += (n))

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())

#define READ_U16() ((u16)((*frame->ip << 8 | frame->ip[1])))

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
    chunk_disassemble_instruction(&frame->pclosure->pfun->chunk, OFFSET());\
  } while (0)
#else
#define PRINT_DEBUG_INFO() (void)0
#endif /* !DEBUG_TRACE_EXECUTION */

  // function body starts here
  Vm *vm = vm_instance();
  CallFrame *frame = vm->frames + vm->frame_count - 1;
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
        vm_stack_push(frame->slots[slot]);
        break;
      }

      case OP_SET_LOCAL: {
        u8 slot = READ_BYTE();
        frame->slots[slot] = *vm_stack_top();
        break;
      }

      case OP_GET_UPVALUE: {
        u8 slot = READ_BYTE();
        vm_stack_push(*frame->pclosure->upvalues[slot]->location);
        break;
      }

      case OP_SET_UPVALUE: {
        u8 slot = READ_BYTE();
        *frame->pclosure->upvalues[slot]->location = *vm_stack_top();
        break;
      }

      case OP_JUMP: {
        u16 offset = READ_U16();
        frame->ip += offset;
        SKIP_BYTES(2);
        break;
      }

      case OP_JUMP_IF_FALSE: {
        u16 offset = READ_U16();
        frame->ip += is_falsey(*vm_stack_top()) * offset;
        SKIP_BYTES(2);
        break;
      }

      case OP_LOOP: {
        u16 offset = READ_U16();
        frame->ip -= offset;
        SKIP_BYTES(2);
        break;
      }

      case OP_CALL: {
        i32 arg_count = READ_BYTE();
        if (!vm_call_value(*vm_stack_peek(arg_count), arg_count)) {
          return INTERPRETER_RUNTUME_ERROR;
        }
        frame = vm->frames + vm->frame_count - 1;
        break;
      }

      case OP_CLOSURE: {
        ObjFunction *pfun = AS_FUNCTION(READ_CONSTANT());
        ObjClosure *pclosure = closure_create(pfun);
        vm_stack_push(OBJ_VAL(pclosure));

        for (i32 i = 0; i < pclosure->upvalue_count; ++i) {
          u8 is_local = READ_BYTE();
          u8 index = READ_BYTE();
          if (is_local) {
            pclosure->upvalues[i] = capture_upvalue(frame->slots + index);
          } else {
            pclosure->upvalues[i] = frame->pclosure->upvalues[index];
          }
        }

        break;
      }

      case OP_RETURN: {
        Value result = vm_stack_pop();

        if (0 == --vm->frame_count) {
          vm_stack_pop();
          return INTERPRET_OK;
        }

        vm->stack_top = frame->slots;
        vm_stack_push(result);

        frame = vm->frames + vm->frame_count - 1;
        break;
      }
    }
  }

#undef PRINT_DEBUG_INFO
#undef BINARY_OP
#undef SKIP_BYTES
#undef READ_U16
#undef READ_STRING
#undef READ_STRING_LONG
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT
#undef READ_BYTE
#undef OFFSET
}


static ObjUpvalue *capture_upvalue(Value *plocal) {
  ObjUpvalue *p_created_upvalue = upvalue_create(plocal);
  return p_created_upvalue;
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

static bool vm_call_value(Value callee, i32 arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_KIND(callee)) {
      case OBJ_CLOSURE:
        return vm_call(AS_CLOSURE(callee), arg_count);

      case OBJ_NATIVE: {
        i32 arity = AS_NATIVE_OBJ(callee)->arity; 
        if (arity != arg_count) {
          runtime_error("Expected %d argumnets, but got %d.",
                        arity, arg_count);
          return false;
        }

        Vm *vm = vm_instance();
        NativeFn native = AS_NATIVE(callee);
        Value result = native(arg_count, vm->stack_top - arg_count);
        vm->stack_top -= arg_count + 1;
        vm_stack_push(result);
        return true;
      }

      default:
        break; // non-callable object
    }
  }

  runtime_error("Can only call functions and classes.");
  return false;
}

static bool vm_call(ObjClosure *pclosure, i32 arg_count) {
  if (arg_count != pclosure->pfun->arity) {
    runtime_error("Expected %d argumnets, but got %d.",
                  pclosure->pfun->arity, arg_count);
    return false;
  }

  Vm *vm = vm_instance();

  if (FRAMES_MAX == vm->frame_count) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame *pframe = vm->frames + vm->frame_count++;
  pframe->pclosure = pclosure;
  pframe->ip = pclosure->pfun->chunk.code;

  pframe->slots = vm->stack_top - arg_count - 1;
  return true;
}

static void native_define(const char *name, NativeFn pfun, i32 arity) {
  Vm *vm = vm_instance();
  vm_stack_push(OBJ_VAL(string_copy(name, (i32)strlen(name))));
  vm_stack_push(OBJ_VAL(native_create(pfun, arity)));
  table_set(&vm->globals, AS_STRING(*vm_stack_peek(1)), *vm_stack_top());
  vm_stack_pop();
  vm_stack_pop();
}

static void runtime_error(const char *format, ...) {
#define ERROR_COLOR COLOR_FG_RED

  Vm *vm = vm_instance();

  //i32 instruction = pframe->ip - pframe->pfun->chunk.code - 1;
  //i32 line = chunk_get_line(&pframe->pfun->chunk, instruction);
  //fprintf(stderr, ERROR_COLOR "[line %d] in script: ", line);

  va_list args;
  va_start(args, format);
  fprintf(stderr, ERROR_COLOR);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (i32 i = vm->frame_count - 1; i > 0; --i) {
    CallFrame *pframe = vm->frames + i;
    ObjFunction *pfun = pframe->pclosure->pfun;

    usize instruction = pframe->ip - pfun->chunk.code - 1;
    i32 line = chunk_get_line(&pfun->chunk, instruction);
    fprintf(stderr, "[line %d] in ", line);

    if (NULL == pfun->name) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", pfun->name->chars);
    }
  }

  fprintf(stderr, COLOR_FG_RESET);
  
  vm_stack_reset();

#undef ERROR_COLOR
}
