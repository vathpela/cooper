extern long int syscall (long int __sysno, ...) __attribute__ ((__nothrow__ , __leaf__));


static const char const _hex[] = "0123456789abcdef";
#define _hi(x) (_hex[(((x) & 0xf0) >> 4)])
#define _lo(x) (_hex[(x) & 0xf])

static void
__attribute__((unused))
dbgs(const char *c)
{
	for (size_t i = 0; c[i] != '\0'; i++)
		syscall(1, 1, c+i, 1);
}

static void
__attribute__((unused))
dbgn(uint32_t num)
{
	char outbuf[11] = "0x";
	int j = 24;
	for (size_t i = 2; i < 11; j-=8) {
		outbuf[i++] = _hi(num >> j);
		outbuf[i++] = _lo(num >> j);
	}
	outbuf[10] = '\0';
	dbgs(outbuf);
}

static void
__attribute__((unused))
dbgsn(char *s, uint32_t num)
{
	dbgs(s);
	dbgn(num);
	dbgs("\n");
}

static void
__attribute__((unused))
dbgc(char s)
{
	char buf[2] = " ";
	buf[0] = s;
	dbgs(buf);
}


