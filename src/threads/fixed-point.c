#include "fixed-point.h"

fixed_t
to_fixed(int n) {
    return n * F;
}

int
to_int_floor(fixed_t x) {
    return x / F;
}

int
to_int_round(fixed_t x) {
    if (x >= 0)
        return (x + F / 2) / F;
    else
        return (x - F / 2) / F;
}

fixed_t
add_fixed(fixed_t x, fixed_t y) {
    return x + y;
}

fixed_t
add_int(fixed_t x, int n) {
    return x + n * F;
}

fixed_t
subtract_fixed(fixed_t x, fixed_t y) {
    return x - y;
}

fixed_t
subtract_int(fixed_t x, int n) {
    return x - n * F;
}

fixed_t
multiply_fixed(fixed_t x, fixed_t y) {
    return ((int64_t)x) * y / F;
}

fixed_t
multiply_int(fixed_t x, int n) {
    return x * n;
}

fixed_t
divide_fixed(fixed_t x, fixed_t y) {
    return ((int64_t)x) * F / y;
}

fixed_t
divide_int(fixed_t x, int n) {
    return x / n;
}
