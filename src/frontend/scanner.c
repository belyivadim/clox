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

/// Checks if current position is at the end of the sequence ('\0')
///
/// @return bool, true if at the end
static bool is_at_end();

/// Advances current position by one
///
/// @return char, character at the position before advance
static char advance();

/// Checks if current character is matched with expected,
/// if it is, then function also advance current position
///
/// @param expected: expected character to match with
/// @return bool, true if there is a match, otherwives false
static bool match(char expected);

/// Creates a Token of passed kind with lexeme starting 
/// at the current position of the scanner 
/// and length is difference between current and start positions
///
/// @param kind: kind of the Token to be created
/// @return Token, newly created Token
static Token token_create(TokenKind kind);

/// Produces Token of kind TOK_ERROR with the message as the lexeme
///
/// @param message: error message, should be available
/// during all the compilation process
/// @return Token, error token
static Token error_token(const char *message);

/// Advances current position while it points to the whitespace symbol,
/// gracefully handles newline characters.
/// Also skips comments
///
/// @return void
static void skip_whitespace();

/// Peeks current character in the scanner
///
/// @return char
static char peek();

/// Peeks the next to current character, if current is the end of file,
/// function will return '\0'
///
/// @return char
static char peek_next();

/// Process string literal and returns token with kind TOK_STRING
/// or TOK_ERROR if it is unterminated string.
/// Gracefully handles newline characters.
///
/// @return Token
static Token process_string();

/// Process number literal and returns token with kind TOK_NUMBER
///
/// @return Token
static Token process_number();

/// Checks if the param char c is a digit
///
/// @param c: character to be checked
/// @return bool, true if it is
static bool is_digit(char c);

/// Checks if the param char c is an alpha or '_'
///
/// @param c: character to be checked
/// @return bool, true if it is
static bool is_alpha(char c);

/// Process identifier and return token with kind TOK_IDENTIFIER
/// if it is user defined identifier
/// or TOK_<keyword> if it is a keyword
///
/// @return Token
static Token process_identifier();

/// Determines kind of the identifier that is being parsed
///
/// @return TokenKind
static TokenKind identifier_kind();

/// Checks if the rest of the word pointed by scanner.current + start param
/// with the length of length param is equal to the rest param
/// if it is, return kind param,
/// otherwise return TOK_IDENTIFIER
///
/// @param start: offset to the beginning of the rest of word pointed by
///   scanner.current
/// @param length: length of the rest of word
/// @param rest: pointer to the rest of the keyword to compare with
/// @param kind: kind of token to return if rest of the word is matched
///   with the rest param
/// @return TokenKind, determined kind of token
static TokenKind check_keyword(i32 start, i32 length,
                              const char *rest, TokenKind kind);


void scanner_init(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scan_token() {
  skip_whitespace();

  scanner.start = scanner.current;

  if (is_at_end()) return token_create(TOK_EOF);

  char c = advance();

  if (is_alpha(c)) return process_identifier();
  if (is_digit(c)) return process_number();

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

    case '!':
      return token_create(match('=') ? TOK_BANG_EQUAL : TOK_BANG);
    case '=':
      return token_create(match('=') ? TOK_EQUAL_EQUAL : TOK_EQUAL);
    case '<':
      return token_create(match('=') ? TOK_LESS_EQUAL : TOK_LESS);
    case '>':
      return token_create(match('=') ? TOK_GREATER_EQUAL : TOK_GREATER);

    case '"': return process_string();

  }

  return error_token("Unexpected character.");
}


static bool is_at_end() {
  return *scanner.current == '\0';
}

static char advance() {
  return *scanner.current++;
}

static bool match(char expected) {
  if (is_at_end() || *scanner.current != expected) return false;

  ++scanner.current;
  return true;
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

static void skip_whitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '\n':
        ++scanner.line;
        advance();
        break;

      case '/':
        if (peek_next() == '/') {
          while (peek() != '\n' && !is_at_end()) advance();
        } else {
          return;
        }
        break;

      default:
        return;
    }
  }
}

static char peek() {
  return *scanner.current;
}

static char peek_next() {
  if (is_at_end()) return '\0';
  return scanner.current[1];
}

static Token process_string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') ++scanner.line;
    advance();
  }

  if (is_at_end()) return error_token("Unterminated string.");

  advance(); // consume closing quote
  return token_create(TOK_STRING);
}

static Token process_number() {
  while (is_digit(peek())) advance();

  // fractional part
  if (peek() == '.' && is_digit(peek_next())) {
    advance(); // consume the dot

    while (is_digit(peek())) advance();
  }

  return token_create(TOK_NUMBER);
}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') 
      || (c >= 'A' && c <= 'Z')
      || c == '_';
}

static Token process_identifier() {
  while (is_alpha(peek()) || is_digit(peek())) advance();

  return token_create(identifier_kind());
}

static TokenKind identifier_kind() {
  switch (*scanner.start) {
    case 'a': return check_keyword(1, 2, "nd", TOK_AND);
    case 'c': return check_keyword(1, 4, "lass", TOK_CLASS);
    case 'e': return check_keyword(1, 3, "lse", TOK_ELSE);
    case 'i': return check_keyword(1, 1, "f", TOK_IF);
    case 'n': return check_keyword(1, 2, "il", TOK_NIL);
    case 'o': return check_keyword(1, 1, "r", TOK_OR);
    case 'p': return check_keyword(1, 4, "rint", TOK_PRINT);
    case 'r': return check_keyword(1, 5, "eturn", TOK_RETURN);
    case 's': return check_keyword(1, 4, "uper", TOK_SUPER);
    case 'v': return check_keyword(1, 2, "ar", TOK_VAR);
    case 'w': return check_keyword(1, 4, "hile", TOK_WHILE);


    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch(scanner.start[1]) {
          case 'a': return check_keyword(2, 3, "lse", TOK_FALSE);
          case 'o': return check_keyword(2, 1, "r", TOK_FOR);
          case 'u': return check_keyword(2, 1, "n", TOK_FUN);
        }
      }
      break;

    case 't':
      if (scanner.current - scanner.start > 1) {
        switch(scanner.start[1]) {
          case 'h': return check_keyword(2, 2, "is", TOK_THIS);
          case 'r': return check_keyword(2, 2, "ue", TOK_TRUE);
        }
      }
      break;
  }

  return TOK_IDENTIFIER;
}

static TokenKind check_keyword(i32 start, i32 length,
                              const char *rest, TokenKind kind) {
  if (scanner.current - scanner.start == start + length
    && 0 == memcmp(scanner.start + start, rest, length)) {
    return kind;
  }

  return TOK_IDENTIFIER;
}

