#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "vm/chunk.h"
#include "vm/debug.h"
#include "vm/vm.h"

#define PRINT_AND_EXIT_IF_TRUE(cond, exit_code, msg_format, ...) \
  do { \
    if (cond) { \
      fprintf(stderr, "[ERR] [LINE: %d] : ", __LINE__); \
      fprintf(stderr, msg_format, __VA_ARGS__); \
      exit(exit_code); \
    } \
  } while (0)

#define PRINT_AND_EXIT_IF_FAIL_PTR(ptr, exit_code, msg_format, ...) \
  PRINT_AND_EXIT_IF_TRUE(NULL == (ptr), exit_code, msg_format, __VA_ARGS__)

/// Runs REPL
/// 
/// @return void
static void repl();

/// Runs file specified by path
///
/// @param path path to the file to be ran
/// @return void
static InterpreterResult run_file(const char *path);

/// Reads content of the file and returns it.
/// Function allocates memory for returning content.
/// The caller gets ownership of this memory, 
/// so caller should free if after usage.
///
/// @param path path to the file to be read
/// @return char*, pointer to the begining of the read content of the file
static char *read_file(const char *path);

enum {
  MAX_REPL_LINE = 1024
};

static void repl() {
  char line[MAX_REPL_LINE];

  for (;;) {
    printf("> ");

    if (!fgets(line, MAX_REPL_LINE, stdin)) {
      printf("\n");
      break;
    }

    vm_interpret(line);
  }
}

static InterpreterResult run_file(const char *path) {
  char *src = read_file(path);
  InterpreterResult result = vm_interpret(src);
  free(src);

  if (INTERPRET_COMPILE_ERROR == result) return 65;
  if (INTERPRETER_RUNTUME_ERROR == result) return 70;
  return 0;
}

static char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");
  PRINT_AND_EXIT_IF_FAIL_PTR(file, 74, 
                             "Not enough memory to open \"%s\".\n", path);

  fseek(file, 0, SEEK_END);
  usize file_size = ftell(file);
  rewind(file);

  char *buff = (char*)malloc(file_size + 1);
  PRINT_AND_EXIT_IF_FAIL_PTR(buff, 74, 
                             "Not enough memory to open \"%s\".\n", path);

  usize bytes_read = fread(buff, sizeof(char), file_size, file);
  PRINT_AND_EXIT_IF_TRUE(bytes_read < file_size, 74,
                         "Could not read file \"%s\".\n", path);

  buff[bytes_read] = '\0';

  fclose(file);
  return buff;
}

int main(int argc, char *argv[])
{
  vm_init();
  InterpreterResult exit_code = 0;

  if (1 == argc) {
    repl();
  } else if (argc == 2) {
    exit_code = run_file(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
  }
  
  vm_free();

  return exit_code;
}
