#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char *source) {
  scanner_init(source);

  i32 line = -1;
  for (;;) {
    Token token = scan_token();

    if (token.line != line) {
      printf("%4d ", token.line);
    } else {
      printf("   | ");
    }

    printf ("%2d '%.*s'\n", token.kind, token.length, token.start);

    if (token.kind == TOK_EOF) break;
  }
}
