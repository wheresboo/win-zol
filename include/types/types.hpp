#pragma once

#include <stdint.h>

// (unsigned) ints
// using int = ???? lol
using uint = unsigned int;

// (unsigned)-pointers
using u8 = uint8_t;
using i8 = int64_t;

using u16 = uint16_t;
using i16 = int64_t;

using u32 = uint32_t;
using i32 = int64_t;

using u64 = uint64_t;
using i64 = int64_t;

struct i128
{
	u64 low;
	i64 high;
};

struct u128
{
	u64 low;
	u64 high;
};

using iptr  = intptr_t;
using uptr = uintptr_t;

// (unsigned)-sizes
using usize = size_t;
using isize = ptrdiff_t;

// floats
using f32 = float;
using f64 = double;
using f80 = long double;
