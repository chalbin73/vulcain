#pragma once

// NOTE: This header contains type definitions

#include <stdint.h>

#include "compiler.h"

// Integer types
// Unsigned
typedef uint64_t    u64;
typedef uint32_t    u32;
typedef uint16_t    u16;
typedef uint8_t     u8;

#define U64_MAX UINT64_MAX
#define U32_MAX UINT32_MAX
#define U16_MAX UINT16_MAX
#define U8_MAX  UINT8_MAX

// Signed
typedef int64_t    i64;
typedef int32_t    i32;
typedef int16_t    i16;
typedef int8_t     i8;

#define I64_MAX INT64_MAX
#define I32_MAX INT32_MAX
#define I16_MAX INT16_MAX
#define I8_MAX  INT8_MAX

// Floating point
typedef float     f32;
typedef double    f64;

// Boolean
typedef int8_t    b8;
#define TRUE  1
#define FALSE 0

// Check type sizes
STATIC_ASSERT(sizeof(u64) == 8, "u64 size is wrong.");
STATIC_ASSERT(sizeof(u32) == 4, "u32 size is wrong.");
STATIC_ASSERT(sizeof(u16) == 2, "u16 size is wrong.");
STATIC_ASSERT(sizeof(u8) == 1, "u8 size is wrong.");

STATIC_ASSERT(sizeof(i64) == 8, "i64 size is wrong.");
STATIC_ASSERT(sizeof(i32) == 4, "i32 size is wrong.");
STATIC_ASSERT(sizeof(i16) == 2, "i16 size is wrong.");
STATIC_ASSERT(sizeof(i8) == 1, "i8 size is wrong.");

STATIC_ASSERT(sizeof(f32) == 4, "f32 size is wrong.");
STATIC_ASSERT(sizeof(f64) == 8, "f64 size is wrong.");

STATIC_ASSERT(sizeof(b8) == 1, "b8 size is wrong.");
STATIC_ASSERT(TRUE, "True has wrong value.");
STATIC_ASSERT(!FALSE, "False has wrong value.");

