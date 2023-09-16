#ifndef __CLOX_NATIVE_TIME_H__
#define __CLOX_NATIVE_TIME_H__

#include "../vm.h"

/// Native wrapper for standard C clock()
Value clock_native(i32 arg_count, Value *args);

#endif // !__CLOX_NATIVE_TIME_H__
