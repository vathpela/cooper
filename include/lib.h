#ifndef COOPER_LIB_H
#define COOPER_LIB_H

#if 0
extern void *(*cooper_malloc)(size_t bytes);
extern void (*cooper_free)(void *addr);

#define malloc(x) ({							\
		void *_ret = NULL;					\
		if (likely(cooper_malloc)) {				\
			if (likely(x)) {				\
				_ret = cooper_malloc(x);		\
			} else {					\
				_ret = 0xabad1dea;			\
			}						\
		_ret;							\
	})

#define free(x) ({							\
		if (likely(cooper_free)) {				\
			if (likely(x) && likely((x) != 0xabad1dea))	\
				cooper_free(x);				\
		}							\
	})
#endif

static inline void
__attribute__((unused))
memset(void *addr, int value, size_t bytes)
{
	off_t o=0;
	if (!addr)
		return;

	if (bytes == ULONG_MAX)
		o=1;
	uint8_t *a = (uint8_t *)addr;
	for (size_t i = 0; i < bytes - o; i++)
		a[i] = value & 0xff;

	if (o)
		a[ULONG_MAX] = value & 0xff;
}

#define mult_will_overflow(x,y,max) ({				\
		int _val = 0;					\
		int _a, _b;					\
		_a = sizeof(x) * 8 - clz(x);			\
		_b = sizeof(y) * 8 - clz(y);			\
		if (_a + _b > popcount(max))			\
			_val = 1;				\
		_val;						\
	})

static inline size_t
__attribute__((unused))
strlen(const char const *s)
{
	if (!s)
		return 0;

	char *e = (char *)s;
	while (e && *e)
		e++;
	return e-s;
}

static inline int
__attribute__((unused))
strncmp(const char const *s0, const char const *s1, size_t max)
{
	size_t i;
	if (!s0 || !s1)
		return s1-s0;
	for (i = 0; i < max && s0[i] && s1[i]; i++)
		if (s0[i] != s1[i])
			return s1[i] - s0[i];
	return 0;
}

#define MIN(x,y) (((x) < (y)) ? (x) : (y))

#if 0
static inline void *
__attribute__((unused))
calloc(size_t nmemb, size_t size)
{
	if (mult_will_overflow(nmemb, size, SIZE_MAX))
		return NULL;

	size_t sz = nmemb * size;
	uint8_t *ret = malloc(sz);
	for (size_t i = 0; ret && i < nmemb; i++)
		memset(ret + size * i, '\0', size);
	return ret;
}
#endif

static inline int
__attribute__((unused))
timespec_zero(struct cooper_timespec *ts)
{
	return ts->tv_sec == 0 && ts->tv_nsec == 0;
}

#endif /* COOPER_LIB_H */
