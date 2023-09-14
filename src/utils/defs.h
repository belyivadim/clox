#ifndef __CLOX_DEFS_H__
#define __CLOX_DEFS_H__

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ssize_t isize;

enum {
  U8_COUNT = UINT8_MAX + 1,
  U16_MAX = UINT16_MAX
};




#endif // !__CLOX_DEFS_H__
