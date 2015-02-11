
#include <decls.h>

static char *databuf;
static size_t databuf_len;
static size_t *databuf_read_pos, *databuf_write_pos;
static cooper_lock_buffer_fn databuf_lock;

static const char const hex[] = "0123456789abcdef";
#define hi(x) (hex[(((x) & 0xf0) >> 4)])
#define lo(x) (hex[(x) & 0xf])

static int16_t
xeh(char c)
{
	c |= 0x20;
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;
	return -1;
}
#define ih(x) (((x) & 0xf) << 4)
#define ol(x) ((x) & 0xf)

static inline int
iscmd(char *str, char *data, size_t len)
{
	return !strncmp(str, data, MIN(strlen(str) + 1, len));
}

static void
cooper_serial_write(char c)
{
	switch (c) {
		case '#':
		case '$':
		case '}':
			cooper_serial_write_raw('}');
			cooper_serial_write_raw(c | 0x20);
			break;
		default:
			cooper_serial_write_raw(c);
	}
}

static int
cooper_send(const char *data, const size_t len)
{
	uint32_t sum = 0;

	cooper_serial_write_raw('$');
	for (size_t i = 0; i < len; i++) {
		sum += data[i];
		cooper_serial_write(data[i]);
	}
	cooper_serial_write_raw('#');
	sum %= 256;
	cooper_serial_write(hi((uint8_t)(sum & 0xff)));
	cooper_serial_write(lo((uint8_t)(sum & 0xff)));

	return 0;
}

static void
__attribute__((unused))
printsum(char c, uint32_t sum)
{
	char buf[3] = " ";
	buf[0] = c;
	dbgs("sum update: '");
	dbgs(buf);
	dbgsn("' ", sum);
}

static int
__attribute__((unused))
cooper_recv(void)
{
	uint8_t c;
	int rc = cooper_serial_read(&c);
	if (rc < 0)
		return rc;
	if (rc == 0)
		return -1;
	if (c == '-' || c == '+') {
		return c == '+' ? 0 : -1;
	}
	if (c != '$') {
protocol_err:
		if (cooper_state.acks)
			cooper_serial_write('-');
		return -1;
	}

	databuf_lock(1);
	if (*databuf_read_pos == *databuf_write_pos) {
		*databuf_read_pos = -1;
		*databuf_write_pos = 0;
	}

	uint32_t sum = 0;
	size_t i = *databuf_write_pos;
	if (i == databuf_len)
		*databuf_write_pos = 0;
	if (*databuf_read_pos == *databuf_write_pos) {
		*databuf_read_pos = -1;
		*databuf_write_pos = 0;
		cooper_serial_write('-');
		return -1;
	}
	databuf[i++] = '$';

	do {
		if (i == databuf_len)
			*databuf_write_pos = 0;
		if (*databuf_read_pos == *databuf_write_pos) {
			*databuf_read_pos = -1;
			*databuf_write_pos = 0;
			cooper_serial_write('-');
			databuf_lock(0);
			return -1;
		}

		rc = cooper_serial_read(&c);
		if (rc <= 0)
			goto protocol_err;
		if (c == '#') {
			databuf[i++] = c;
			break;
		}
		if (c == '}') {
			databuf[i++] = c;
			rc = cooper_serial_read(&c);
			if (rc <= 0)
				goto protocol_err;
			databuf[i++] = c;
			c &= ~0x20;
			sum += c;
		} else {
			databuf[i++] = c;
			sum += c;
		}
	} while (1);
	sum %= 256;
	uint32_t cmpsum = 0;
	rc = cooper_serial_read(&c);
	if (rc <= 0)
		goto protocol_err;
	cmpsum |= ih(xeh(c));
	rc = cooper_serial_read(&c);
	if (rc <= 0)
		goto protocol_err;
	cmpsum |= ol(xeh(c));

	if (cmpsum != sum) {
		dbgs("cmpsum is wrong: ");
		dbgn(cmpsum);
		dbgsn(" vs ", sum);
		goto protocol_err;
	}

	(*databuf_write_pos) += i;
	//dbgsn("*databuf_write_pos: ", *databuf_write_pos);
	databuf_lock(0);

	if (i && cooper_state.acks)
		cooper_serial_write('+');
	return i;
}

void
cooper_console_out(const char *str)
{
	char s[strlen(str)+2];
	s[0] = 'O';
	strcpy(s+1, str);
	cooper_send(s, sizeof(s)-1);
}

static void
handle_big_h(char *data, size_t len)
{
	cooper_send("OK", 2);
}

static void
handle_big_o(char *data, size_t len)
{
	cooper_send("OK", 2);
}

static void
handle_tracepoints(char *data, size_t len)
{
	if (iscmd("qTStatus", data, len)) {
		char ret[] = "T0;tnotrun:0";
		cooper_send(ret, sizeof(ret)-1);
		return;
	}
	if (iscmd("qTsV", data, len)) {
		char ret[] = "";
		cooper_send(ret, sizeof(ret)-1);
		return;
	}
	if (iscmd("qTfV", data, len)) {
		char ret[] = "";
		cooper_send(ret, sizeof(ret)-1);
		return;
	}
	cooper_send("E01", 2);
}

static void
handle_cmd(char *data, size_t len)
{
	char s[len+1];
	memcpy(s, data, len);
	s[len] = '\0';
	dbgs("cmd: ");
	dbgs(s);
	dbgs("\n");
	switch (data[0]) {
	case '!': /* extended mode */
		break;
	case '?':
		cooper_send("W FF", 4);
		return;
	case 'H':
		handle_big_h(data, len);
		return;
	case 'O':
		if (iscmd("OK", data, len))
			return;
		handle_big_o(data, len);
		return;
	case 'q':
		if (iscmd("qSupported", data, len)) {
			char supported[] = "qSupported:QStrtNoAckMode";
			cooper_send(supported, strlen(supported));
			return;
		} else if (iscmd("qT", data, len)) {
			handle_tracepoints(data, len);
			return;
		}
		cooper_send("OK", 2);
		return;
	case 'Q':
		if (iscmd("QStartNoAckMode", data, len))
			cooper_state.acks = 0;
		cooper_send("OK", 2);
		return;
	}
	cooper_send("E01", 3);
}

int
cooper_service_buffer(void)
{
	databuf_lock(1);
	int rc = 0;
	off_t lp, rp = 0;
	size_t ls, rs = 0;

	if (*databuf_read_pos > databuf_len)
		*databuf_read_pos = 0;
	if (*databuf_read_pos == *databuf_write_pos) {
		databuf_lock(0);
		return 0;
	}
	if (*databuf_read_pos < 0)
		*databuf_read_pos = 0;
	lp = *databuf_read_pos;
	if (*databuf_write_pos > *databuf_read_pos)
		ls = *databuf_write_pos - *databuf_read_pos;
	if (*databuf_write_pos < *databuf_read_pos) {
		rp = *databuf_write_pos;
		rs = *databuf_write_pos;
	}

	char s[databuf_len];

	memcpy(s, databuf+lp, ls);
	memcpy(s+ls, databuf+rp, rs);

	size_t begin = -1;
	for (size_t i = 0; i < ls+rs; i++) {
		if (begin < 0) {
			if (s[i] == '+') {
				dbgs("plus leaked\n");
				break;
			}
			if (s[i] == '-') {
				dbgs("minus leaked\n");
				break;
			}
		}
		if (s[i] == '$')
			begin = i+1;
		if (begin >= 0 && begin < i && s[i] == '#') {
			(*databuf_read_pos) += i + 1;
			if (*databuf_read_pos > databuf_len)
				(*databuf_read_pos) -= databuf_len;
			rc = i-begin;
			databuf_lock(0);
			handle_cmd(s+begin, i-begin);
			begin = -1;
			continue;
		}
	}
	return rc;
}

void
cooper_service_serial(cooper_timer_id_t timer_id, void *cbdata)
{
	int rc = 0;

	rc = cooper_recv();
	if (rc < 0)
		dbgsn("cooper_recv rc: ", rc);
	if (rc > 0)
		cooper_service_buffer();
}

int
cooper_set_serial_buffer(void *addr, size_t size,
			 cooper_size_t *read_pos, cooper_size_t *write_pos,
			 cooper_lock_buffer_fn lock)
{
	databuf = (char *)addr;
	databuf_len = size;
	databuf_read_pos = read_pos;
	databuf_write_pos = write_pos;
	databuf_lock = lock;

	lock(1);
	*read_pos = 0;
	*write_pos = 0;
	lock(0);
	return 0;
}
