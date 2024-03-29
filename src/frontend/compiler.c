#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "frontend/token.h"
#include "scanner.h"
#include "vm/chunk.h"
#include "../vm/object.h"
#include "../utils/memory.h"

#include "../vm/debug.h"

/// Represent the Token Parser
typedef struct {
  /// Represent the current Token the Parser is parsing at the moment
  Token current;

  /// Represent the last parsed Token
  Token previous;

  /// The flag that specifies if an error has been occured 
  /// during parsing/compiling process
  bool had_error;

  /// The flag that specifies if the parser currently is in the panic mode.
  /// While parser is in the panic mode it does not report errors.
  bool panic_mode;
} Parser;

/// Represents the precedence of an expression
typedef enum {
  PREC_NONE,
  PREC_ASSIGMENT,
  PREC_OR,
  PREC_AND,
  PREC_EQUALITY,
  PREC_COMPARISON,
  PREC_TERM,          // + -
  PREC_FACTOR,        // * /
  PREC_UNARY,
  PREC_CALL,          // . ()
  PREC_PRIMARY
} Precedence;

/// Represents the type of a function for parse rules table
typedef void (*ParseFn)(bool can_assign);

/// Represents the parse rule in parse table
typedef struct {
  /// The function for prefix expression
  ParseFn prefix;

  /// The function for infix expression
  ParseFn infix;

  /// The precedence of the rule
  Precedence precedence;
} ParseRule;

/// Represents the local variable at compile time
typedef struct {
  /// Name of the local variable
  Token name;

  /// depth of the scope where a variable is declared
  i32 depth;

  /// tracks if variable is captured by a closure
  bool is_captured;
} Local;

typedef struct {
  u8 index;
  bool is_local;
} Upvalue;

typedef enum {
  FUN_KIND_FUNCTION,
  FUN_KIND_INITIALIZER,
  FUN_KIND_METHOD,
  FUN_KIND_SCRIPT
} FunKind;

/// Represents a compile time related data
typedef struct Compiler {
  /// Enclosing compiler
  struct Compiler *penclosing;

  /// Currently compiled function
  ObjFunction *pfun;

  /// Kind of the currently compiled function
  FunKind fun_kind;

  /// Array of currently defined local variables
  Local locals[U8_COUNT];

  /// Number of active elements in the locals array
  i32 local_count;

  /// Current scope deapth
  i32 scope_depth;

  /// Array of currently define upvalues
  Upvalue upvalues[U8_COUNT];
} Compiler;

typedef struct ClassCompiler {
  /// Pointer to enclosing compiler
  struct ClassCompiler *penclosing;

  /// Class name
  Token name;

  /// Flag is set if class is derived
  bool has_super_class;
} ClassCompiler;


/// Parser singleton
Parser parser;

/// pointer to the current innermost class being compiled
ClassCompiler *current_class = NULL;

/// pointer to the instance of the currently active compiler
Compiler *current_compiler = NULL;

/// The getter for the chunk that is currently being compiled
///
/// @return Chunk*
static Chunk *current_chunk() {
  return &current_compiler->pfun->chunk;
}

/// Initializes currently active compiler to the compiler param
///
/// @param compiler: pointer to the compiler instance, current_compiler should be initialized to,
///   also sets up all fields of compiler to zeros
/// @return void
static void compiler_init(Compiler *compiler, FunKind fun_kind) {
  compiler->penclosing = current_compiler;
  compiler->fun_kind = fun_kind;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  compiler->pfun = function_create();
  current_compiler = compiler;

  if (FUN_KIND_SCRIPT != fun_kind) {
    current_compiler->pfun->name
      = string_copy(parser.previous.start, parser.previous.length);
  }

  // implicitly claim stack slot 0 for the VM's own internal use
  // with name of an empty string, so user cannot refer to it
  Local *local = current_compiler->locals + current_compiler->local_count++;
  local->depth = 0;
  local->is_captured = false;
  if (fun_kind != FUN_KIND_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

/// Advances the scanner by scanning the next token
///
/// @return void
static void advance();

static void expression();

/// Writes the byte to the current chunk
///
/// @param byte: the byte to be written
static void emit_byte(u8 byte);

/// Emits the OP_RETURN to the currently compiled chunk
///
/// @return void
static void emit_return();

/// Emits OP_CONSTANT* opcode with operand base on value param
///
/// @param value: value to emit
/// @return i32, index to the value stored in the current chunk's constatnts array
static i32 emit_constant(Value value);

/// Checks if the current token is of kind param,
/// if it is then advances the parser,
/// otherwise reports an error at the current token with the message param
///
/// @param kind: expected kind of the current token
/// @param message: message for the error in case kind does not match
/// @return void
static void consume(TokenKind kind, const char *message);

/// Ends the compilation process
///
/// @return ObjFunction*, pointer to the compiled function 
static ObjFunction* end_compiler();

/// Reports to stderr that an error has been occured at the current token
///
/// @param message: a message that describes the error
/// @return void
static void error_at_current(const char *message);

/// Reports to stderr that an error has been occured at the token param
///
/// @param token: a token error has been occured at
/// @param message: a message that describes the error
/// @return void
static void error_at(const Token *token, const char *message);

/// Reports to stderr that an error has been occured at the just consumed token
///
/// @param message: a message that describes the error
/// @return void
static void error(const char *message);


/// Handles number expression,
/// requires parser's previous field to be of TOK_NUMBER kind
///
/// @return void
static void number_handler(bool can_assign);

/// Handles grouping expression,
/// requires parser's previous field to be of TOK_LEFT_PAREN kind
///
/// @return void
static void grouping_handler(bool can_assign);

/// Handles unary expression,
/// requires parser's previous field to be of TOK_MINUS or TOK_BANG kind
///
/// @return void
static void unary_handler(bool can_assign);

/// Handles binary expression,
/// requires parser's previous field to be of 
/// TOK_MINUS, TOK_PLUS, TOK_SLASH or TOK_STAR kind
///
/// @return void
static void binary_handler(bool can_assign);

/// Handles literal expression,
/// requires parser's previous field to be of 
/// TOK_FALSE, TOK_TRUE, or TOK_NIL kind
///
/// @return void
static void literal_hanler(bool can_assign);

/// Handles string expression,
/// requires parser's previous field to be of 
/// TOK_STRING kind
///
/// @return void
static void string_handler(bool can_assign);

/// Handles variable expression
///
/// @return void
static void variable_handler(bool can_assign);

/// Handles call expression
///
/// @return void
static void call_handler(bool can_assign);

static void and_handler(bool can_assign);
static void or_handler(bool can_assign);
static void dot_handler(bool can_assign);
static void this_handler(bool can_assign);
static void super_handler(bool can_assign);

/// Parses argument list for call expression
///
/// @return u8, number of arguments
static u8 argument_list();

/// Writes ObjString based on the lexeme in name param as Value to the current chunk's constants array,
///   and emits OP_<GET/SET>_GLOBAL* opcode with index of that value
///
/// @param name: pointer to the token of kind TOK_IDENTIFIER
/// @param can_assign, flag that specifies if current context is allowed to perform assign operation
/// return void
static void named_variable(const Token *name, bool can_assign);

/// Looking for a closest to current scope upvalue
///
/// @param compiler: pointer to the current compiler
/// @param name: pointer to the token with value name to resolve
/// @return i32, upvalue index, or -1 1 if not found
static i32 resolve_upvalue(Compiler *compiler, const Token *name);

static i32 add_upvalue(Compiler *compiler, u8 index, bool is_local);


static void declaration_handler();
static void statement_handler();
static void print_st_handler();
static void expr_stmt_hanler();
static void var_decl_handler();
static void if_statement_handler();
static void while_statement_handler();
static void for_statement_handler();
static void fun_decl_handler();
static void return_statement_handler();
static void class_decl_handler();

static void method();


/// Creates token with .start points to the text param
/// and .length equal to length of the string pointed by text param
///
/// Note: does not initialize .kind, and .line fields
///
/// @return Token, syntheticly create token
static Token synthetic_token(const char *text);

/// Compiles the function
///
/// @param fun_kind: kind of the function to complie
/// @return void
static void function(FunKind fun_kind);

/// Emits jump instruction with 2 byte placeholder for jump operand
///
/// @param instruction: jump instruction
/// @return i32, offset from the beginning of byte code to placeholder
///   for operand of emited jump instruction
static i32 emit_jump(u8 instruction);

/// Emits loop instruction with 2 byte operand
///
/// @param loop_start: index in current chunk's code where to jump
/// @return void
static void emit_loop(i32 loop_start);

/// Patches jump operand to actual value,
///   how many bytes to skip 
///
/// @param offset: offset to the jump operand in current chunk's code
/// @return void
static void patch_jump(i32 offset);

/// Begins the new lexical scope
///
/// @return void
static void begin_scope();

/// Ends the last lexical scope, discards all of its locals
///
/// @return void
static void end_scope();

/// Handles block statement
///
/// @return void
static void block();


/// Parses variable identintifier.
///   Sets parser in panic mode if it cannot parse the variable identintifier,
///   and reports an error with the error_msg param
/// 
/// @param error_msg: error message to report in case of error
/// @return i32, index of the variable identifier in chunks constants array
static i32 parse_variable(const char *error_msg);

/// Writes ObjString as Value to the current chunk's constants array
///
/// @param name: pointer to the token of kind TOK_IDENTIFIER
/// @return i32, index of the variable identifier in chunks constants array
static i32 identifier_constant(const Token *name);

/// Emits VM's opcode with the name with index param
///   where index param is index of constant value in the current chunk's constants array
///
/// @param param: param up to 24-bits
/// @return void
static void emit_opcode_with_param(OpCode opcode, i32 param);

/// Marks variable as initialized if it is local variable.
/// Emmits OP_DEFINE_GLOBAL with global operand if it is global variable
///
/// @param global: index of the global variable in the currently compiled chunk's
///   constants array
/// @return void
static void define_variable(i32 global);

/// Declares variable in local scope 
///   by adding it to the current_compiler's locals.
/// If current scope is global, does nothing.
///
/// @return void
static void declare_variable();

/// Adds name param to the current_compiler's locals array
///   with depth of the current depth
///
/// @param name: pointer to the Token with the name of the local variable to add
static void add_local(const Token *name);

/// Resolves local variable, returns index of variable's value on the stack
///
/// @param compiler: pointer to the current compiler
/// @param name: pointer to the token with variable's name
/// @return i32, index of the variable's value on the stack,
///   or -1 if not found
static i32 resolve_local(const Compiler *compiler, const Token *name);

/// Marks currently compiled variable as initialized
///
/// @return void
static void mark_initialized();

/// Compares lexemes in tokens
///
/// @param lhs, rhs: tokens to compare
/// @return bool, true if lexemes are equal, false otherwise
static bool identifiers_equal(const Token *lhs, const Token *rhs);


/// Checks if parser's current token is of the same kind as kind param
///   advances if it is
///
/// @param kind: kind to compare with
/// @return bool, true if matched, false otherwise
static bool match(TokenKind kind);

/// Checks if parser's current token is of the same kind as kind param
///
/// @param kind: kind to compare with
/// @return bool, true if passed the check, false otherwise
static bool check(TokenKind kind);

/// Synchronizes parser's state after entering panic_mode
///   by skipping all the tokens until encounter an synchronization point
///   (such as semicolon or control flow statement) or TOK_EOF
///
/// @return void
static void synchronize();

/// Parses all the subsequent expression with the precedence
/// equal or higher than precedence param
///
/// @param precedence: the minimum precedence of the expression to be parsed
/// @return void
static void parse_precedence(Precedence precedence);

/// Getter for a rule from rules table
///
/// @param kind: kind of a token to get rule for
/// @return ParseRule*, pointer to the ParseRule in rules table
static ParseRule *get_rule(TokenKind kind);


/// The table of parse rules
ParseRule rules[] = {
  [TOK_LEFT_PAREN]    = {grouping_handler, call_handler, PREC_CALL},
  [TOK_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
  [TOK_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
  [TOK_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
  [TOK_COMMA]         = {NULL, NULL, PREC_NONE},
  [TOK_DOT]           = {NULL, dot_handler, PREC_CALL},
  [TOK_MINUS]         = {unary_handler, binary_handler, PREC_TERM},
  [TOK_PLUS]          = {NULL, binary_handler, PREC_TERM},
  [TOK_SEMICOLON]     = {NULL, NULL, PREC_NONE},
  [TOK_SLASH]         = {NULL, binary_handler, PREC_FACTOR},
  [TOK_STAR]          = {NULL, binary_handler, PREC_FACTOR},
  [TOK_BANG]          = {unary_handler, NULL, PREC_NONE},
  [TOK_BANG_EQUAL]    = {NULL, binary_handler, PREC_EQUALITY},
  [TOK_EQUAL_EQUAL]   = {NULL, binary_handler, PREC_EQUALITY},
  [TOK_GREATER]       = {NULL, binary_handler, PREC_COMPARISON},
  [TOK_GREATER_EQUAL] = {NULL, binary_handler, PREC_COMPARISON},
  [TOK_LESS]          = {NULL, binary_handler, PREC_COMPARISON},
  [TOK_LESS_EQUAL]    = {NULL, binary_handler, PREC_COMPARISON},
  [TOK_IDENTIFIER]    = {variable_handler, NULL, PREC_NONE},
  [TOK_STRING]        = {string_handler, NULL, PREC_NONE},
  [TOK_NUMBER]        = {number_handler, NULL, PREC_NONE},
  [TOK_AND]           = {NULL, and_handler, PREC_AND},
  [TOK_ELSE]          = {NULL, NULL, PREC_NONE},
  [TOK_FALSE]         = {literal_hanler, NULL, PREC_NONE},
  [TOK_FOR]           = {NULL, NULL, PREC_NONE},
  [TOK_FUN]           = {NULL, NULL, PREC_NONE},
  [TOK_IF]            = {NULL, NULL, PREC_NONE},
  [TOK_NIL]           = {literal_hanler, NULL, PREC_NONE},
  [TOK_OR]            = {NULL, or_handler, PREC_OR},
  [TOK_PRINT]         = {NULL, NULL, PREC_NONE},
  [TOK_RETURN]        = {NULL, NULL, PREC_NONE},
  [TOK_SUPER]         = {super_handler, NULL, PREC_NONE},
  [TOK_THIS]          = {this_handler, NULL, PREC_NONE},
  [TOK_TRUE]          = {literal_hanler, NULL, PREC_NONE},
  [TOK_VAR]           = {NULL, NULL, PREC_NONE},
  [TOK_WHILE]         = {NULL, NULL, PREC_NONE},
  [TOK_ERROR]         = {NULL, NULL, PREC_NONE},
  [TOK_EOF]           = {NULL, NULL, PREC_NONE},

};




ObjFunction *compile(const char *source) {
  assert(NULL != source);

  scanner_init(source);

  Compiler compiler;
  compiler_init(&compiler, FUN_KIND_SCRIPT);

  parser.had_error = false;
  parser.panic_mode = false;

  advance();
  while (!match(TOK_EOF)) {
    declaration_handler();
  }

  ObjFunction *pfun = end_compiler();

  return parser.had_error ? NULL : pfun;
}

static void declaration_handler() {
  if (match(TOK_CLASS)) {
    class_decl_handler();
  } else if (match(TOK_FUN)) {
    fun_decl_handler();
  } else if (match(TOK_VAR)) {
    var_decl_handler();
  } else {
    statement_handler();
  }

  if (parser.panic_mode) {
    synchronize();
  }
}

static void statement_handler() {
  if (match(TOK_PRINT)) {
    print_st_handler();
  } else if (match(TOK_IF)) {
    if_statement_handler();
  } else if (match(TOK_FOR)) {
    for_statement_handler();
  } else if (match(TOK_RETURN)) {
    return_statement_handler();
  } else if (match(TOK_WHILE)) {
    while_statement_handler();
  } else if (match(TOK_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expr_stmt_hanler();
  }
}

static void print_st_handler() {
  expression();
  consume(TOK_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_PRINT);
}

static void expr_stmt_hanler() {
  expression();
  consume(TOK_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_POP);
}

static void var_decl_handler() {
  u32 global = parse_variable("Expect variable name");

  if (match(TOK_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }

  consume(TOK_SEMICOLON, "Expect ';' after variable declaration");

  define_variable(global);
}

static void class_decl_handler() {
  consume(TOK_IDENTIFIER, "Expect class name.");
  Token class_name = parser.previous;
  i32 name_constant = identifier_constant(&parser.previous);
  declare_variable();

  emit_opcode_with_param(OP_CLASS, name_constant);
  define_variable(name_constant);

  ClassCompiler class_compiler;

  class_compiler.name = parser.previous;
  class_compiler.has_super_class = false;
  class_compiler.penclosing = current_class;
  current_class = &class_compiler;

  if (match(TOK_LESS)) {
    consume(TOK_IDENTIFIER, "Expect supreclass name.");
    variable_handler(false);

    if (identifiers_equal(&class_name, &parser.previous)) {
      error("A class can't inherit from itself.");
    }

    // super keyword
    begin_scope(); // need new scope, so other class decls "super" would not collide
    Token super_ = synthetic_token("super");
    add_local(&super_);
    define_variable(0);

    named_variable(&class_name, false); // push class onto the stack
    emit_byte(OP_INHERIT);
    class_compiler.has_super_class = true;
  }

  named_variable(&class_name, false); // push class onto the stack
  consume(TOK_LEFT_BRACE, "Expect '{' before class body");
  while (!check(TOK_RIGHT_BRACE) && !check(TOK_EOF)) {
    method();
  }
  consume(TOK_RIGHT_BRACE, "Expect '}' after class body");
  emit_byte(OP_POP); // pop class from the stack

  // closing scope for "super" keyword
  if (class_compiler.has_super_class) {
    end_scope();
  }

  current_class = current_class->penclosing;
}

static Token synthetic_token(const char *text) {
  return (Token){
    .start = text,
    .length = (i32)strlen(text)
  };
}

static void method() {
  consume(TOK_IDENTIFIER, "Expect method name.");
  i32 constant = identifier_constant(&parser.previous);

  FunKind kind = FUN_KIND_METHOD;
  if (4 == parser.previous.length
    && 0 == memcmp(parser.previous.start, "init", 4)) {
    kind = FUN_KIND_INITIALIZER;
  }
  function(kind);

  emit_opcode_with_param(OP_METHOD, constant);
}

static void if_statement_handler() {
  consume(TOK_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOK_RIGHT_PAREN, "Expect ')' after 'if'.");

  i32 then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP); // pop result of condition expression from the stack
  statement_handler();

  i32 else_jump = emit_jump(OP_JUMP);
  patch_jump(then_jump);
  emit_byte(OP_POP); // pop result of condition expression from the stack

  if (match(TOK_ELSE)) statement_handler();
  patch_jump(else_jump);
}

static void while_statement_handler() {
  i32 loop_start = current_chunk()->code_count;

  consume(TOK_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOK_RIGHT_PAREN, "Expect ')' after 'if'.");

  i32 exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement_handler();

  emit_loop(loop_start);
  patch_jump(exit_jump);
  emit_byte(OP_POP);
}

static void for_statement_handler() {
  begin_scope();

  consume(TOK_LEFT_PAREN, "Expect '(' after 'for'.");

  if (match(TOK_SEMICOLON)) {
    // no initializer
  } else if (match(TOK_VAR)) {
    var_decl_handler();
  } else {
    expr_stmt_hanler();
  }

  i32 loop_start = current_chunk()->code_count;

  i32 exit_jump = -1;
  if (!match(TOK_SEMICOLON)) {
    expression();
    consume(TOK_SEMICOLON, "Expect ';' after loop condition");

    // jump out of the loop if the condition is false
    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }

  if (!match(TOK_RIGHT_PAREN)) {
    i32 body_jump = emit_jump(OP_JUMP);

    i32 increment_start = current_chunk()->code_count;
    expression();
    emit_byte(OP_POP);
    consume(TOK_RIGHT_PAREN, "Exprect ')' after for clauses");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement_handler();

  emit_loop(loop_start);

  if (-1 != exit_jump) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  end_scope();
}

static void fun_decl_handler() {
  i32 global = parse_variable("Expect function name.");
  mark_initialized();
  function(FUN_KIND_FUNCTION);
  define_variable(global);
}

static void function(FunKind fun_kind) {
  Compiler compiler;
  compiler_init(&compiler, fun_kind);
  begin_scope();

  // param list
  consume(TOK_LEFT_PAREN, "Expect '(' after function name.");

  if (!check(TOK_RIGHT_PAREN)) {
    do {
      if (++current_compiler->pfun->arity > 255) {
        error_at_current("Cannot have more than 255 parameters.");
      }

      i32 param_constant = parse_variable("Expect parameter name.");
      define_variable(param_constant);
    } while (match(TOK_COMMA));
  }

  consume(TOK_RIGHT_PAREN, "Expect ')' after function name.");

  // body
  consume(TOK_LEFT_BRACE, "Expect '{' before function body.");
  block();

  // create ObjFunction
  ObjFunction *pfun = end_compiler();
  // at the moment there is no OP_CLOSURE_LONG, so it will cause bag 
  // if chunk_add_constant will return index more than 255
  i32 fun_index = chunk_add_constant(current_chunk(), OBJ_VAL(pfun));
  assert(fun_index < 255 && "Does not suppord OP_CLOSURE_LONG.");
  emit_opcode_with_param(OP_CLOSURE, fun_index);

  for (i32 i = 0; i < pfun->upvalue_count; ++i) {
    emit_byte((u8)compiler.upvalues[i].is_local);
    emit_byte(compiler.upvalues[i].index);
  }
}

static void return_statement_handler() {
  if (FUN_KIND_SCRIPT == current_compiler->fun_kind) {
    error("Cannot return from top-level code");
  }

  if (match(TOK_SEMICOLON)) {
    emit_return();
  } else {
    if (FUN_KIND_INITIALIZER == current_compiler->fun_kind) {
      error("Can't return a value from an initializer.");
    }

    expression();
    consume(TOK_SEMICOLON, "Expect ';' after return value");
    emit_byte(OP_RETURN);
  }
}

static void and_handler(bool can_assign) {
  (void)can_assign;
  i32 end_jump = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP); // pop result of lhs expression from the stack
  parse_precedence(PREC_AND);

  patch_jump(end_jump);
}

static void or_handler(bool can_assign) {
  (void)can_assign;
  i32 else_jump = emit_jump(OP_JUMP_IF_FALSE);
  i32 end_jump = emit_jump(OP_JUMP);

  patch_jump(else_jump);
  emit_byte(OP_POP);

  parse_precedence(PREC_OR);
  patch_jump(end_jump);
}


static i32 emit_jump(u8 instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_chunk()->code_count - 2;
}

static void patch_jump(i32 offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  i32 jump = current_chunk()->code_count - offset - 2;

  if (jump > U16_MAX) {
    error("Too much code to jump over.");
  }

  current_chunk()->code[offset] = (jump >> 8) & 0xff;
  current_chunk()->code[offset + 1] = (jump) & 0xff;
}

static void emit_loop(i32 loop_start) {
  emit_byte(OP_LOOP);

  i32 offset = current_chunk()->code_count - loop_start + 2;
  if (offset > U16_MAX) error("Loop body is too large");

  emit_byte((offset >> 8) & 0xff);
  emit_byte((offset) & 0xff);
}

static i32 parse_variable(const char *error_msg)  {
  consume(TOK_IDENTIFIER, error_msg);

  declare_variable();
  if (current_compiler->scope_depth > 0) return -1;

  return identifier_constant(&parser.previous);
}

static i32 identifier_constant(const Token *name) {
  assert(TOK_IDENTIFIER == name->kind);
  return chunk_add_constant(current_chunk(), OBJ_VAL(string_copy(name->start, name->length)));
}

static void emit_opcode_with_param(OpCode opcode, i32 param) {
  if (param < 256) {
    emit_byte(opcode);
    emit_byte((u8)param);
  } else {
    emit_byte((OpCode)(opcode + 1));
    emit_byte((u8)(param >> 16));
    emit_byte((u8)(param >> 8));
    emit_byte((u8)(param));
  }
}

static void define_variable(i32 global) {
  if (current_compiler->scope_depth > 0) {
    mark_initialized();
    return;
  }

  emit_opcode_with_param(OP_DEFINE_GLOBAL, global);
}

static void declare_variable() {
  // global variables are implicitly declared
  if (current_compiler->scope_depth == 0) return;

  Token *name = &parser.previous;

  for (i32 i = current_compiler->local_count - 1; i >= 0; --i) {
    Local *local = current_compiler->locals + i;

    if (-1 != local->depth && local->depth < current_compiler->scope_depth) {
      break;
    }

    if (identifiers_equal(name, &local->name)) {
      error("Already variable with this name in this scope.");
    }
  }

  add_local(name);
}

static void add_local(const Token *name) {
  assert(NULL != current_compiler);

  if (U8_COUNT == current_compiler->local_count) {
    error("To many local variables in the scope.");
    return;
  }

  Local *local = current_compiler->locals + current_compiler->local_count++;
  local->name = *name;
  local->depth = -1; // sential value during intializer compilation
  local->is_captured = false;
}

static bool identifiers_equal(const Token *lhs, const Token *rhs) {
  return lhs->length == rhs->length
        && 0 == memcmp(lhs->start, rhs->start, lhs->length);
}

static void block() {
  while (!check(TOK_RIGHT_BRACE) && !check(TOK_EOF)) {
    declaration_handler();
  }

  consume(TOK_RIGHT_BRACE, "Expect '}' after block.");
}

static void begin_scope() {
  ++current_compiler->scope_depth;
}

static void end_scope() {
  --current_compiler->scope_depth;

  while (current_compiler->local_count > 0
      && current_compiler->locals[current_compiler->local_count - 1].depth > current_compiler->scope_depth) {

    if (current_compiler->locals[current_compiler->local_count - 1].is_captured) {
      emit_byte(OP_CLOSE_UPVALUE);
    } else {
      emit_byte(OP_POP);
    }

    --current_compiler->local_count;
  }
}

static void synchronize() {
  parser.panic_mode = false;

  while (parser.current.kind != TOK_EOF) {
    if (TOK_SEMICOLON == parser.previous.kind) return;

    switch (parser.current.kind) {
      case TOK_CLASS:
      case TOK_FUN:
      case TOK_VAR:
      case TOK_FOR:
      case TOK_IF:
      case TOK_WHILE:
      case TOK_PRINT:
      case TOK_RETURN:
        return;

      default:
        ; // do nothing
    }

    advance();
  }
}

static bool match(TokenKind kind) {
  if (!check(kind)) return false;
  advance();
  return true;
}

static bool check(TokenKind kind) {
  return kind == parser.current.kind;
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();
    if (parser.current.kind != TOK_ERROR) break;

    error_at_current(parser.current.start);
  }
}

static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

static void error_at(const Token *token, const char *message) {
#define ERROR_COLOR COLOR_FG_RED

  if (parser.panic_mode) return;
  parser.panic_mode = true;

  fprintf(stderr, ERROR_COLOR "[line %d] Error", token->line);

  if (token->kind == TOK_EOF) {
    fprintf(stderr, " at end");
  } else if (token->kind == TOK_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n" COLOR_FG_RESET, message);

  parser.had_error = true;

#undef ERROR_COLOR
}

static void error(const char *message) {
  error_at(&parser.previous, message);
}

static void consume(TokenKind kind, const char *message) {
  if (parser.current.kind == kind) {
    advance();
    return;
  }

  error_at_current(message);
}

static ObjFunction* end_compiler() {
  emit_return(); 
  ObjFunction *pfun = current_compiler->pfun;

#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    chunk_disassemble(current_chunk(), 
                      NULL != pfun->name ? pfun->name->chars : "<script>");
  }
#endif // !DEBUG_PRINT_CODE

  current_compiler = current_compiler->penclosing;
  return pfun;
}

static void emit_byte(u8 byte) {
  chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_return() {
  if (FUN_KIND_INITIALIZER == current_compiler->fun_kind) {
    emit_opcode_with_param(OP_GET_LOCAL, 0);
  } else {
    emit_byte(OP_NIL);
  }

  emit_byte(OP_RETURN);
}

static i32 emit_constant(Value value) {
  return chunk_write_constant(current_chunk(), value, parser.previous.line);
}

static void expression() {
  parse_precedence(PREC_ASSIGMENT);
}


static void number_handler(bool can_assign) {
  (void)can_assign;
  assert(TOK_NUMBER == parser.previous.kind);
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void grouping_handler(bool can_assign) {
  (void)can_assign;
  assert(TOK_LEFT_PAREN == parser.previous.kind);
  expression();
  consume(TOK_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary_handler(bool can_assign) {
  (void)can_assign;
  TokenKind operator_kind = parser.previous.kind;

  assert(TOK_MINUS == operator_kind || TOK_BANG == operator_kind);

  // compile the operand
  parse_precedence(PREC_UNARY);

  switch (operator_kind) {
    case TOK_MINUS: emit_byte(OP_NEGATE); break;
    case TOK_BANG: emit_byte(OP_NOT); break;

    default:
      return; // unreachable
  }
}

static void binary_handler(bool can_assign) {
  (void)can_assign;
  TokenKind operator_kind = parser.previous.kind;

  // compile the right operand
  ParseRule *rule = get_rule(operator_kind);
  // +1 because all of supported binary operators are right associative,
  // so we should not capture expression with the same precedence
  // to the right operand
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (operator_kind) {
    case TOK_PLUS:          emit_byte(OP_ADD); break;
    case TOK_MINUS:         emit_byte(OP_SUBSTRACT); break;
    case TOK_STAR:          emit_byte(OP_MULTIPLY); break;
    case TOK_SLASH:         emit_byte(OP_DIVIDE); break;

    case TOK_BANG_EQUAL:    emit_byte(OP_NOT_EQUAL); break;
    case TOK_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
    case TOK_GREATER:       emit_byte(OP_GREATER); break;
    case TOK_GREATER_EQUAL: emit_byte(OP_GREATER_EQUAL); break;
    case TOK_LESS:          emit_byte(OP_LESS); break;
    case TOK_LESS_EQUAL:    emit_byte(OP_LESS_EQUAL); break;

    default:
      assert(false && "Wrong operator kind.");
      return; // unreachable
  }
}

static void literal_hanler(bool can_assign) {
  (void)can_assign;
  TokenKind kind = parser.previous.kind;
  assert(TOK_NIL == kind || TOK_FALSE == kind || TOK_TRUE == kind);

  switch (kind) {
    case TOK_FALSE: emit_byte(OP_FALSE); break;
    case TOK_NIL: emit_byte(OP_NIL); break;
    case TOK_TRUE: emit_byte(OP_TRUE); break;

    default:
      return; // unreachable
  }
}

static void string_handler(bool can_assign) {
  (void)can_assign;
  // emits string literal with trimed quotes
  emit_constant(OBJ_VAL(string_copy(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

static void call_handler(bool can_assign) {
  (void)can_assign;
  u8 arg_count = argument_list();
  emit_opcode_with_param(OP_CALL, arg_count);
}

static u8 argument_list() {
  u8 arg_count = 0;

  if (!check(TOK_RIGHT_PAREN)) {
    do {
      expression();

      if (255 == arg_count) {
        error("Can't have more than 255 arguments.");
      }

      ++arg_count;
    } while (match(TOK_COMMA));
  }

  consume(TOK_RIGHT_PAREN, "Expect ')' after arguments.");
  return arg_count;
}

static void variable_handler(bool can_assign) {
  named_variable(&parser.previous, can_assign);
}

static void dot_handler(bool can_assign) {
  consume(TOK_IDENTIFIER, "Expect property name after '.'.");
  i32 name = identifier_constant(&parser.previous);

  if (can_assign && match(TOK_EQUAL)) {
    expression();
    emit_opcode_with_param(OP_SET_PROPERTY, name);
  } else if (match(TOK_LEFT_PAREN)) {
    // immediate call, no need to create bound method
    // compile call expression and emit OP_INVOKE
    u8 arg_count = argument_list();
    emit_opcode_with_param(OP_INVOKE, name);
    emit_byte(arg_count);
  } else {
    emit_opcode_with_param(OP_GET_PROPERTY, name);
  }
}

static void this_handler(bool can_assign) {
  (void)can_assign;

  if (NULL == current_class) {
    error("Can't use 'this' outside of a class.");
    return;
  }

  variable_handler(false);
}

static void super_handler(bool can_assign) {
  (void)can_assign;
  if (NULL == current_class) {
    error("Can't use 'super' outside of a class.");
  } else if (!current_class->has_super_class) {
    error("Can't user 'super' in a class with no superclass");
  }

  consume(TOK_DOT, "Expect '.' after 'super'.");
  consume(TOK_IDENTIFIER, "Expect superclass method name");
  i32 name = identifier_constant(&parser.previous);

  Token this_ = synthetic_token("this");
  named_variable(&this_, false);
  Token super_ = synthetic_token("super");
  
  if (match(TOK_LEFT_PAREN)) {
    u8 argc = argument_list();
    named_variable(&super_, false);
    emit_opcode_with_param(OP_SUPER_INVOKE, name);
    emit_byte(argc);
  } else {
    named_variable(&super_, false);
    emit_opcode_with_param(OP_GET_SUPER, name);
  }
}

static void named_variable(const Token *name, bool can_assign) {
  u8 get_op, set_op;
  i32 param = resolve_local(current_compiler, name); 

  if (-1 != param) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else if (-1 != (param = resolve_upvalue(current_compiler, name))) {
    get_op = OP_GET_UPVALUE;
    set_op = OP_SET_UPVALUE;
  } else {
    param = identifier_constant(name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOK_EQUAL)) {
    expression();
    emit_opcode_with_param(set_op, param);
  } else {
    emit_opcode_with_param(get_op, param);
  }
}

static i32 resolve_upvalue(Compiler *compiler, const Token *name) {
  if (NULL == compiler->penclosing) return -1;

  i32 local = resolve_local(compiler->penclosing, name);

  if (-1 != local) {
    current_compiler->penclosing->locals[local].is_captured = true;
    return add_upvalue(compiler, (u8)local, true);
  }

  i32 upvalue = resolve_upvalue(compiler->penclosing, name);
  if (-1 != upvalue) {
    return add_upvalue(compiler, (u8)upvalue, false);
  }

  return -1;
}


static i32 resolve_local(const Compiler *compiler, const Token *name) {
  for (i32 i = compiler->local_count - 1; i >= 0; --i) {
    const Local *local = compiler->locals + i;
    if (identifiers_equal(name, &local->name)) {
      if (-1 == local->depth) {
        error("Can't read local variable in its own initializer.");
      }

      return i;
    }
  }

  return -1;
}

static i32 add_upvalue(Compiler *compiler, u8 index, bool is_local) {
  i32 upvalue_count = compiler->pfun->upvalue_count;

  for (i32 i = 0; i < upvalue_count; ++i) {
    Upvalue *upvalue = compiler->upvalues + i;
    if (upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  }

  if (U8_COUNT == upvalue_count) {
    error("Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalue_count].is_local = is_local;
  compiler->upvalues[upvalue_count].index = index;
  return compiler->pfun->upvalue_count++;
}

static void mark_initialized() {
  if (0 == current_compiler->scope_depth) return;

  current_compiler->locals[current_compiler->local_count - 1].depth
    = current_compiler->scope_depth;
}

static void parse_precedence(Precedence precedence) {
  advance();
  ParseFn prefix_rule = get_rule(parser.previous.kind)->prefix;

  if (NULL == prefix_rule) {
    error("Expect expression.");
    return;
  }

  bool can_assign = precedence <= PREC_ASSIGMENT;

  prefix_rule(can_assign);

  while (precedence <= get_rule(parser.current.kind)->precedence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.kind)->infix;
    assert(NULL != infix_rule);
    infix_rule(can_assign);
  }

  if (can_assign && match(TOK_EQUAL)) {
    error("Invalid assigment target.");
  }
}

static ParseRule *get_rule(TokenKind kind) {
  return &rules[kind];
}


void mark_compiler_roots() {
  Compiler *compiler = current_compiler;
  while (NULL != compiler) {
    mark_object((Obj*)compiler->pfun);
    compiler = compiler->penclosing;
  }
}
