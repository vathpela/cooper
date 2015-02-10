#ifndef COOPER_API_H
#define COOPER_API_H

/* sys/types.h */
typedef uint64_t cooper_size_t;
typedef uint32_t cooper_time_t;
struct cooper_timespec {
	cooper_time_t tv_sec;
	int32_t tv_nsec;
};

extern int cooper_init(void);

typedef void (*cooper_timer_cb)(int timer, void *cbdata);
typedef int (*cooper_set_timer_fn)(cooper_timer_cb timer_cb,
				struct cooper_timespec frequency,
				void *cbdata);
extern int cooper_set_timer_cb(cooper_set_timer_fn set_timer);
extern int cooper_delete_timer(int timer);

typedef void (*cooper_break_handler)(int breakpoint, void *cbdata);
typedef int (*cooper_set_breakpoint_fn)(cooper_break_handler bh,
				     void *addr, void *cbdata);
extern int cooper_set_breakpoint_cb(cooper_set_breakpoint_fn sbp);

typedef int (*cooper_read_u8)(void *addr, uint8_t *val);
typedef int (*cooper_read_u16)(void *addr, uint16_t *val);
typedef int (*cooper_read_u32)(void *addr, uint32_t *val);
typedef int (*cooper_read_u64)(void *addr, uint64_t *val);
typedef int (*cooper_write_u8)(void *addr, uint8_t val);
typedef int (*cooper_write_u16)(void *addr, uint16_t val);
typedef int (*cooper_write_u32)(void *addr, uint32_t val);
typedef int (*cooper_write_u64)(void *addr, uint64_t val);
extern int cooper_set_io(cooper_read_u8 ru8, cooper_read_u16 ru16,
			 cooper_read_u32 ru32, cooper_read_u64 ru64,
			 cooper_write_u8 wu8, cooper_write_u16 wu16,
			 cooper_write_u32 wu32, cooper_write_u64 wu64);

typedef uint8_t (*cooper_serial_read_fn)(void);
typedef void (*cooper_serial_write_fn)(uint8_t c);
extern int cooper_set_serial(cooper_serial_read_fn read,
			     cooper_serial_write_fn write,
			     struct cooper_timespec service_interval);

extern int cooper_start_debugging(void);

#endif /* COOPER_API_H */
