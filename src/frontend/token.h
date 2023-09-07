#ifndef __CLOX_TOKEN_H__
#define __CLOX_TOKEN_H__

#include "common.h"

typedef enum {
  // Single-character tokens.
  TOK_LEFT_PAREN, TOK_RIGHT_PAREN,
  TOK_LEFT_BRACE, TOK_RIGHT_BRACE,
  TOK_COMMA, TOK_DOT, TOK_MINUS, TOK_PLUS,
  TOK_SEMICOLON, TOK_SLASH, TOK_STAR,

  // One or two character tokens.
  TOK_BANG, TOK_BANG_EQUAL,
  TOK_EQUAL, TOK_EQUAL_EQUAL,
  TOK_GREATER, TOK_GREATER_EQUAL,
  TOK_LESS, TOK_LESS_EQUAL,

  // Literals.
  TOK_IDENTIFIER, TOK_STRING, TOK_NUMBER,

  // Keywords.
  TOK_AND, TOK_CLASS, TOK_ELSE, TOK_FALSE,
  TOK_FOR, TOK_FUN, TOK_IF, TOK_NIL, TOK_OR,
  TOK_PRINT, TOK_RETURN, TOK_SUPER, TOK_THIS,
  TOK_TRUE, TOK_VAR, TOK_WHILE,

  // Specials.
  TOK_ERROR,
  TOK_EOF

} TokenKind;

/// Represents the Token of source code lexeme
typedef struct {
  /// Kind of the Token
  TokenKind kind;

  /// Pointer to the beginning of the lexeme in source code.
  /// Token does not own this memory, 
  /// it simply points to the place in source string
  const char *start;

  /// Length of the lexeme
  i32 length;

  /// Number of the line, on which lexeme represented by this token
  /// is located
  i32 line;
} Token;

#endif // !__CLOX_TOKEN_H__
