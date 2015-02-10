#ifndef COOPER_TYPES_H
#define COOPER_TYPES_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <builtins.h>

#if 0
/* stddef.h */
typedef long int ptrdiff_t;

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

#endif

/* sys/types.h */
typedef long int off_t;
typedef unsigned long size_t;
typedef long int ssize_t;

#endif /* COOPER_TYPES_H */
