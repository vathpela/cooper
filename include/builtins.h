#ifndef COOPER_BUILTINS_H
#define COOPER_BUILTINS_H

#define clrsb(x) (__builtin_clrsb(x))
#define clz(x) (__builtin_clz(x))
#define ctz(x) (__builtin_ctz(x))
#define ffs(x) (__builtin_ffs(x))
#define ffsl(x) (__builtin_ffsl(x))
#define popcount(x) (__builtin_popcount((signed long)(unsigned long)x))
#define strcpy(d,s) (__builtin_strcpy ((d),(s)))
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#define memcpy(x,y,z) (__builtin_memcpy(x,y,z))

#endif /* COOPER_BUILTINS_H */
