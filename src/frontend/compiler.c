#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "frontend/token.h"
#include "scanner.h"
#include "vm/chunk.h"
#include "../vm/object.h"

#ifdef DEBUG_PRINT_CODE
#include "../vm/debug.h"
#endif // !DEBUG_PRINT_CODE

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
typedef void (*ParseFn)();

/// Represents the parse rule in parse table
typedef struct {
  /// The function for prefix expression
  ParseFn prefix;

  /// The function for infix expression
  ParseFn infix;

  /// The precedence of the rule
  Precedence precedence;
} ParseRule;


/// Parser singleton
Parser parser;

/// Represents a pointer to the chunk that is currently compiled
Chunk *compiling_chunk;

/// The getter for the chunk that is currently being compiled
///
/// @return Chunk
static Chunk *current_chunk() {
  return compiling_chunk;
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
/// @return void
static void end_compiler();

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
static void number_handler();

/// Handles grouping expression,
/// requires parser's previous field to be of TOK_LEFT_PAREN kind
///
/// @return void
static void grouping_handler();

/// Handles unary expression,
/// requires parser's previous field to be of TOK_MINUS or TOK_BANG kind
///
/// @return void
static void unary_handler();

/// Handles binary expression,
/// requires parser's previous field to be of 
/// TOK_MINUS, TOK_PLUS, TOK_SLASH or TOK_STAR kind
///
/// @return void
static void binary_handler();

/// Handles literal expression,
/// requires parser's previous field to be of 
/// TOK_FALSE, TOK_TRUE, or TOK_NIL kind
///
/// @return void
static void literal_hanler();

/// Handles string expression,
/// requires parser's previous field to be of 
/// TOK_STRING kind
///
/// @return void
static void string_handler();

static void variable_handler();

/// Writes ObjString based on the lexeme in name param as Value to the current chunk's constants array,
///   and emits OP_GET_GLOBAL* opcode with index of that value
///
/// @param name: pointer to the token of kind TOK_IDENTIFIER
/// return void
static void named_variable(const Token *name);


static void declaration_handler();
static void statement_handler();
static void print_st_handler();
static void expression_st_hanler();
static void var_decl_handler();


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
static i32 identintifier_constant(const Token *name);

/// Emits VM's opcode with the name with index param
///   where index param is index of constant value in the current chunk's constants array
///
/// @param index: index in current chunk's constants value array 
/// @return void
static void emit_opcode_with_constant_param(OpCode opcode, i32 index);


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
  [TOK_LEFT_PAREN]    = {grouping_handler, NULL, PREC_NONE},
  [TOK_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
  [TOK_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
  [TOK_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
  [TOK_COMMA]         = {NULL, NULL, PREC_NONE},
  [TOK_DOT]           = {NULL, NULL, PREC_NONE},
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
  [TOK_AND]           = {NULL, NULL, PREC_NONE},
  [TOK_ELSE]          = {NULL, NULL, PREC_NONE},
  [TOK_FALSE]         = {literal_hanler, NULL, PREC_NONE},
  [TOK_FOR]           = {NULL, NULL, PREC_NONE},
  [TOK_FUN]           = {NULL, NULL, PREC_NONE},
  [TOK_IF]            = {NULL, NULL, PREC_NONE},
  [TOK_NIL]           = {literal_hanler, NULL, PREC_NONE},
  [TOK_OR]            = {NULL, NULL, PREC_NONE},
  [TOK_PRINT]         = {NULL, NULL, PREC_NONE},
  [TOK_RETURN]        = {NULL, NULL, PREC_NONE},
  [TOK_SUPER]         = {NULL, NULL, PREC_NONE},
  [TOK_THIS]          = {NULL, NULL, PREC_NONE},
  [TOK_TRUE]          = {literal_hanler, NULL, PREC_NONE},
  [TOK_VAR]           = {NULL, NULL, PREC_NONE},
  [TOK_WHILE]         = {NULL, NULL, PREC_NONE},
  [TOK_ERROR]         = {NULL, NULL, PREC_NONE},
  [TOK_EOF]           = {NULL, NULL, PREC_NONE},

};




bool compile(const char *source, Chunk *chunk) {
  assert(NULL != source);
  assert(NULL != chunk);

  scanner_init(source);

  compiling_chunk = chunk;

  parser.had_error = false;
  parser.panic_mode = false;

  advance();
  while (!match(TOK_EOF)) {
    declaration_handler();
  }

  end_compiler();

  return !parser.had_error;
}

static void declaration_handler() {
  if (match(TOK_VAR)) {
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
  } else {
    expression_st_hanler();
  }
}

static void print_st_handler() {
  expression();
  consume(TOK_SEMICOLON, "Expect ';' after value.");
  emit_byte(OP_PRINT);
}

static void expression_st_hanler() {
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

  emit_opcode_with_constant_param(OP_DEFINE_GLOBAL, global);
}


static i32 parse_variable(const char *error_msg)  {
  consume(TOK_IDENTIFIER, error_msg);
  return identintifier_constant(&parser.previous);
}

static i32 identintifier_constant(const Token *name) {
  assert(TOK_IDENTIFIER == name->kind);
  return chunk_add_constant(current_chunk(), OBJ_VAL(string_copy(name->start, name->length)));
}

static void emit_opcode_with_constant_param(OpCode opcode, i32 index) {
  if (index < 256) {
    emit_byte(opcode);
    emit_byte((u8)index);
  } else {
    emit_byte((OpCode)(opcode + 1));
    emit_byte((u8)(index >> 16));
    emit_byte((u8)(index >> 8));
    emit_byte((u8)(index));
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
  if (parser.panic_mode) return;
  parser.panic_mode = true;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->kind == TOK_EOF) {
    fprintf(stderr, " at end");
  } else if (token->kind == TOK_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);

  parser.had_error = true;
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

static void end_compiler() {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    chunk_disassemble(current_chunk(), "code");
  }
#endif // !DEBUG_PRINT_CODE
}

static void emit_byte(u8 byte) {
  chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_return() {
  emit_byte(OP_RETURN);
}

static i32 emit_constant(Value value) {
  return chunk_write_constant(current_chunk(), value, parser.previous.line);
}

static void expression() {
  parse_precedence(PREC_ASSIGMENT);
}


static void number_handler() {
  assert(TOK_NUMBER == parser.previous.kind);
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void grouping_handler() {
  assert(TOK_LEFT_PAREN == parser.previous.kind);
  expression();
  consume(TOK_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary_handler() {
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

static void binary_handler() {
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

static void literal_hanler() {
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

static void string_handler() {
  // emits string literal with trimed quotes
  emit_constant(OBJ_VAL(string_copy(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

static void variable_handler() {
  named_variable(&parser.previous);
}

static void named_variable(const Token *name) {
  i32 param = identintifier_constant(name);
  emit_opcode_with_constant_param(OP_GET_GLOBAL, param);
}

static void parse_precedence(Precedence precedence) {
  advance();
  ParseFn prefix_rule = get_rule(parser.previous.kind)->prefix;

  if (NULL == prefix_rule) {
    error("Expect expression.");
    return;
  }

  prefix_rule();

  while (precedence <= get_rule(parser.current.kind)->precedence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.kind)->infix;
    assert(NULL != infix_rule);
    infix_rule();
  }
}

static ParseRule *get_rule(TokenKind kind) {
  return &rules[kind];
}
