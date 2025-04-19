#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>

typedef int fixed_t;

#define Q 14
#define F (1 << Q)

// Conversion functions
fixed_t
to_fixed(int n);

int
to_int_floor(fixed_t x);

int
to_int_round(fixed_t x);

// Addition
fixed_t
add_fixed(fixed_t x, fixed_t y);

fixed_t
add_int(fixed_t x, int n);

// Subtraction
fixed_t
subtract_fixed(fixed_t x, fixed_t y);

fixed_t
subtract_int(fixed_t x, int n);

// Multiplication
fixed_t
multiply_fixed(fixed_t x, fixed_t y);

fixed_t
multiply_int(fixed_t x, int n);

// Division
fixed_t
divide_fixed(fixed_t x, fixed_t y);

fixed_t
divide_int(fixed_t x, int n);

#endif
