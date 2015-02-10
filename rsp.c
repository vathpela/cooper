
#include <decls.h>

static uint8_t databuf[4096];
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

static void
cooper_serial_write(uint8_t c)
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
cooper_send(uint8_t cmd, const char *data, const size_t len)
{
	uint32_t sum = cmd;

	cooper_serial_write_raw('$');
	cooper_serial_write(cmd);
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

static int
__attribute__((unused))
cooper_recv(uint8_t *cmd, uint8_t **data_ret, size_t *len_ret)
{
	if (!cmd || !data_ret | !len_ret)
		return -1;

	char c = cooper_serial_read();
	if (c == '-' || c == '+')
		return 0;
	if (c != '$') {
err:
		cooper_serial_write('-');
		return -1;
	}

	c = cooper_serial_read();
	*cmd = c;

	uint32_t sum = c;
	size_t i = 0;

	for(i = 0; i < 4096; i++) {
		c = cooper_serial_read();
		if (c == '#')
			break;
		if (c == '}') {
			c = cooper_serial_read();
			c &= ~0x20;
		}
		databuf[i] = c;
		sum += c;
	}
	if (i == 4096)
		goto err;
	sum %= 256;
	uint32_t cmpsum = 0;
	c = cooper_serial_read();
	cmpsum |= ih(xeh(c));
	c = cooper_serial_read();
	cmpsum |= ol(xeh(c));

	if (cmpsum != sum)
		goto err;

	*data_ret = databuf;
	*len_ret = i;

	//cooper_serial_write('+');
	return 0;
}

void
cooper_console_out(const char *str)
{
	cooper_send('O', str, strlen(str));
}

void
cooper_service_serial(int timer, void *cbdata)
{
	int rc;
	uint8_t cmd;
	uint8_t *data = NULL;
	size_t len = 0;
	char err[] = "$E01#01";
	char okay[] = "+ $#00";
	char *ret = okay;

	rc = cooper_recv(&cmd, &data, &len);
	if (rc < 0) {
		ret = err;
		cooper_serial_write_raw('-');
		return;
	}

	cooper_serial_write_raw('+');
	cooper_send('q', "Supported:multiprocess+;xmlRegisters=i386;qRelocInsn+", strlen("Supported:multiprocess+;xmlRegisters=i386;qRelocInsn+"));
	//cooper_send('E', (uint8_t *)"01", 2);
	return;
	for (int i = 0; i < strlen(ret); i++)
		cooper_serial_write_raw(ret[i]);
}
