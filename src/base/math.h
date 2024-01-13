#pragma once

#define MAX(a, b)        ( (a) < (b) ? b : a )
#define MIN(a, b)        ( (a) < (b) ? a : b )

// Clamps x under a
#define CLAMP_UP(x, a)   ( MIN(x, a) )

// Calmps x over a
#define CLAMP_DOWN(x, a) ( MAX(x, a) )

// Clamps x between a and b
#define CLAMP(x, a, b)   ( CLAMP_DOWN(CLAMP_UP(x, b), a) )

