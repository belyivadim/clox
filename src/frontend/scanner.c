#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/// Represent the source code scanner
typedef struct {
  /// Pointer to the beginning of the token that is currently being parsed
  const char *start;

  /// Pointer to the current location in source code 
  const char *current;

  /// Number of the current line in source file
  i32 line;
} Scanner;

/// scanner singleton
Scanner scanner;

static bool is_at_end();
static char advance();

static Token token_create(TokenKind kind);

/// Produces Token of kind TOK_ERROR with the message as the lexeme
///
/// @param message: error message, should be available
/// during all the compilation process
/// @return Token, error token
static Token error_token(const char *message);


void scanner_init(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scan_token() {
  scanner.start = scanner.current;

  if (is_at_end()) return token_create(TOK_EOF);

  char c = advance();

  switch (c) {
    case '(': return token_create(TOK_LEFT_PAREN);
    case ')': return token_create(TOK_RIGHT_PAREN);
    case '{': return token_create(TOK_LEFT_BRACE);
    case '}': return token_create(TOK_RIGHT_BRACE);
    case ';': return token_create(TOK_SEMICOLON);
    case ',': return token_create(TOK_COMMA);
    case '.': return token_create(TOK_DOT);
    case '-': return token_create(TOK_MINUS);
    case '+': return token_create(TOK_PLUS);
    case '/': return token_create(TOK_SLASH);
    case '*': return token_create(TOK_STAR);
  }

  return error_token("Unexpected character.");
}


static bool is_at_end() {
  return *scanner.current == '\0';
}

static char advance() {
  return *scanner.current++;
}

static Token token_create(TokenKind kind) {
  return (Token) {
    .kind = kind,
    .start = scanner.start,
    .length = (i32)(scanner.current - scanner.start),
    .line = scanner.line
  };
}

static Token error_token(const char *message) {
  return (Token) {
    .kind = TOK_ERROR,
    .start = message,
    .length = (i32)strlen(message),
    .line = scanner.line
  };
}
