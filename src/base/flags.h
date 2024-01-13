#pragma once

#define MK_FLAG(n)           (1 << n)
#define IS_FLAGS(d, f)       ( (d & f) == f )
#define CONTAINS_FLAGS(a, b) (a & b)
#define IS_FLAG_N(f, b)      (a & (1 << b)

