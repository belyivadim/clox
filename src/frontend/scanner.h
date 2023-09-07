#ifndef __CLOX_SCANNER_H__
#define __CLOX_SCANNER_H__

#include "token.h"

/// Initializes the Scanner to scan source code
///
/// @param source: pointer to the source code
/// @return void
void scanner_init(const char *source);

/// Scans the next token in the current source file
///
/// @return Token, scanned token
Token scan_token();


#endif // !__CLOX_SCANNER_H__
