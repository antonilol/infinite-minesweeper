#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define BIT(n) (1 << (n))
#define SET(flag, flags) ((flags) |= (flag))
#define UNSET(flag, flags) ((flags) &= ~(flag))
#define TOGGLE(flag, flags) ((flags) ^= (flag))
#define ISSET(flag, flags) ((flags) & (flag))

void handle_alloc_error();

#endif
