#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "frontend/token.h"
#include "scanner.h"

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
  [TOK_BANG]          = {NULL, NULL, PREC_NONE},
  [TOK_BANG_EQUAL]    = {NULL, NULL, PREC_NONE},
  [TOK_GREATER]       = {NULL, NULL, PREC_NONE},
  [TOK_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
  [TOK_LESS]          = {NULL, NULL, PREC_NONE},
  [TOK_LESS_EQUAL]    = {NULL, NULL, PREC_NONE},
  [TOK_IDENTIFIER]    = {NULL, NULL, PREC_NONE},
  [TOK_STRING]        = {NULL, NULL, PREC_NONE},
  [TOK_NUMBER]        = {number_handler, NULL, PREC_NONE},
  [TOK_AND]           = {NULL, NULL, PREC_NONE},
  [TOK_ELSE]          = {NULL, NULL, PREC_NONE},
  [TOK_FALSE]         = {NULL, NULL, PREC_NONE},
  [TOK_FOR]           = {NULL, NULL, PREC_NONE},
  [TOK_FUN]           = {NULL, NULL, PREC_NONE},
  [TOK_IF]            = {NULL, NULL, PREC_NONE},
  [TOK_NIL]           = {NULL, NULL, PREC_NONE},
  [TOK_OR]            = {NULL, NULL, PREC_NONE},
  [TOK_PRINT]         = {NULL, NULL, PREC_NONE},
  [TOK_RETURN]        = {NULL, NULL, PREC_NONE},
  [TOK_SUPER]         = {NULL, NULL, PREC_NONE},
  [TOK_THIS]          = {NULL, NULL, PREC_NONE},
  [TOK_TRUE]          = {NULL, NULL, PREC_NONE},
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
  expression();
  consume(TOK_EOF, "Expect end of expression.");

  end_compiler();

  return !parser.had_error;
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
}

static void emit_byte(u8 byte) {
  chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_return() {
  emit_byte(OP_RETURN);
}

static void expression() {
  parse_precedence(PREC_ASSIGMENT);
}


static void number_handler() {
  assert(TOK_NUMBER == parser.previous.kind);
  double value = strtod(parser.previous.start, NULL);
  chunk_write_constant(current_chunk(), value, parser.previous.line);
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
    case TOK_BANG: break;

    default:
      return; // unreachable
  }
}

static void binary_handler() {
  TokenKind operator_kind = parser.previous.kind;

  assert(operator_kind == TOK_MINUS || operator_kind == TOK_PLUS
         || operator_kind == TOK_SLASH || operator_kind == TOK_STAR);

  // compile the right operand
  ParseRule *rule = get_rule(operator_kind);
  // +1 because all of supported binary operators are right associative,
  // so we should not capture expression with the same precedence
  // to the right operand
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (operator_kind) {
    case TOK_PLUS:    emit_byte(OP_ADD); break;
    case TOK_MINUS:    emit_byte(OP_SUBSTRACT); break;
    case TOK_STAR:    emit_byte(OP_MULTIPLY); break;
    case TOK_SLASH:    emit_byte(OP_DIVIDE); break;

    default:
      return; // unreachable
  }
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
