#ifndef TYPES_H
#define TYPES_H

#include <float.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

typedef int b32;

#define S8_MAX CHAR_MAX
#define S8_MIN CHAR_MIN
#define S16_MAX SHRT_MAX
#define S16_MIN SHRT_MIN
#define S32_MAX INT_MAX
#define S32_MIN INT_MIN
#define S64_MAX LLONG_MAX
#define S64_MIN LLONG_MIN

#define U8_MAX 0xFF
#define U8_MIN 0
#define U16_MAX USHRT_MAX
#define U16_MIN USHRT_MIN
#define U32_MAX UINT_MAX
#define U32_MIN UINT_MIN
#define U64_MAX ULLONG_MAX
#define U64_MIN 0

#define F32_MAX FLT_MAX
#define F32_MIN FLT_MIN
#define F64_MAX DBL_MAX
#define F64_MIN DBL_MIN

#endif //TYPES_H
