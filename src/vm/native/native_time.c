#include <time.h>

#include "native_time.h"

Value clock_native(i32 arg_count, Value *args) {
  (void)arg_count;
  (void)args;
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
