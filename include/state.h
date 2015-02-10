#ifndef COOPER_STATE_H
#define COOPER_STATE_H

typedef struct _cooper_state {
	int serial_timer;
	struct cooper_timespec service_interval;

	int acks;
} cooper_state_t;

extern cooper_state_t cooper_state;
extern cooper_set_timer_fn cooper_set_timer;
extern cooper_serial_read_fn cooper_serial_read;
extern cooper_serial_write_fn cooper_serial_write_raw;

#endif /* COOPER_STATE_H */
