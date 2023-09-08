#include <stdio.h>

#include "common.h"
#include "compiler.h"
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

}
