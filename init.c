
#include <decls.h>

cooper_state_t cooper_state = {
	.serial_timer = -1,
	.service_interval = { 0, 200000 },
	.acks = 0,
};
cooper_set_timer_fn cooper_set_timer = 0;
cooper_serial_read_fn cooper_serial_read = 0;
cooper_serial_write_fn cooper_serial_write_raw = 0;

#if 0
cooper_set_breakpoint_fn cooper_set_break = 0;
cooper_read_u8 cooper_read_u8 = 0;
cooper_read_u16 cooper_read_u16 = 0;
cooper_read_u32 cooper_read_u32 = 0;
cooper_read_u64 cooper_read_u64 = 0;
cooper_write_u8 cooper_write_u8 = 0;
cooper_write_u16 cooper_write_u16 = 0;
cooper_write_u32 cooper_write_u32 = 0;
cooper_write_u64 cooper_write_u64 = 0;
#endif

int
cooper_init(void)
{
	memset(&cooper_state, '\0', sizeof cooper_state);
	cooper_state.serial_timer = -1;
	return 0;
}

int
cooper_set_timer_cb(cooper_set_timer_fn set_timer)
{
	static int x;
	cooper_set_timer = set_timer;
	return x++;
}

#if 0
int
cooper_set_breakpoint_cb(cooper_set_breakpoint_fn sbp)
{
	cooper_set_break = sbp;
	return 0;
}

int
cooper_set_io(cooper_read_u8 ru8, cooper_read_u16 ru16,
	      cooper_read_u32 ru32, cooper_read_u64 ru64,
	      cooper_write_u8 wu8, cooper_write_u16 wu16,
	      cooper_write_u32 wu32, cooper_write_u64 wu64)
{
	cooper_read_u8 = ru8;
	cooper_read_u16 = ru16;
	cooper_read_u32 = ru32;
	cooper_read_u64 = ru64;
	cooper_write_u8 = wu8;
	cooper_write_u16 = wu16;
	cooper_write_u32 = wu32;
	cooper_write_u64 = wu64;
	return 0;
}
#endif

int
cooper_set_serial(cooper_serial_read_fn read, cooper_serial_write_fn write,
		  struct cooper_timespec service_interval)
{
	cooper_serial_read = read;
	cooper_serial_write_raw = write;
	cooper_state.service_interval = service_interval;
	return 0;
}

int
cooper_start_debugging(void)
{
	if (!cooper_serial_read)
		return -1;
	if (!cooper_serial_write_raw)
		return -1;
	if (!cooper_set_timer && !timespec_zero(&cooper_state.service_interval))
		return -1;

	cooper_state.serial_timer = cooper_set_timer(cooper_service_serial,
						cooper_state.service_interval,
						NULL);
	cooper_console_out("What fresh hell is this?");
	return 0;
}
