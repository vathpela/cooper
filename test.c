
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "include/api.h"

#define COOPER_NETWORK

static int
__attribute__((unused))
set_up_socket(void)
{
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
		err(1, "unable to create socket");

	struct sockaddr_in addr_in = {
		.sin_family = AF_INET,
	};
	inet_pton(AF_INET, "127.0.0.1", &addr_in.sin_addr);
	addr_in.sin_port = htons(2159);

	int rc = bind(sd, (struct sockaddr *)&addr_in, sizeof(addr_in));
	if (rc < 0)
		err(2, "unable to bind to port");

	rc = listen(sd, 5);
	if (rc < 0)
		err(3, "unable to listen on socket");

	return sd;
}

struct timer_data {
	cooper_timer_cb cb;
	cooper_timer_id_t timer_id;
	void *priv;
};
static struct timer_data sigdata = { NULL, 0, NULL };

static void timer_dispatcher(int signal, siginfo_t *siginfo, void *ucontext)
{
	sigdata.cb(sigdata.timer_id, sigdata.priv);
}

static int
set_timer(cooper_timer_id_t *timer_id,
	  cooper_timer_cb cb,
	  struct cooper_timespec *frequency,
	  void *cbdata)
{
	struct sigevent sev = {{ 0 }};
	struct itimerspec its = {{ 0 }};
	struct itimerspec its_zero = {{ 0 }};
	sigset_t mask;
	struct sigaction sa = {{ 0 }};
	int rc = 0;

	if (timer_id == NULL) {
		printf("timer_id is null\n");
		errno = EINVAL;
		return -1;
	}

	sa.sa_sigaction = timer_dispatcher;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = timer_id;

	if (frequency) {
		its.it_interval.tv_sec = frequency->tv_sec;
		its.it_interval.tv_nsec = frequency->tv_nsec;
		its.it_value.tv_sec = frequency->tv_sec;
		its.it_value.tv_nsec = frequency->tv_nsec;
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);

	if (cb != NULL && frequency != NULL) {
		rc = sigaction(SIGALRM, &sa, NULL);

		rc = timer_create(CLOCK_MONOTONIC, &sev, timer_id);
		if (rc < 0)
			return rc;

		rc = timer_settime(*timer_id, 0, &its, NULL);
		if (rc < 0)
			goto err_timer_create;

		rc = sigprocmask(SIG_UNBLOCK, &mask, NULL);
		if (rc >= 0) {
			sigdata.cb = cb;
			sigdata.timer_id = *timer_id;
			sigdata.priv = cbdata;
			return rc;
		}
	}

	sigprocmask(SIG_BLOCK, &mask, NULL);
err_timer_create:
	timer_settime(*timer_id, TIMER_ABSTIME, &its_zero, NULL);
	timer_delete(*timer_id);

	return rc;
}

int sock = -1;

static void close_socket(int signal, siginfo_t *siginfo, void *ucontext)
{
	if (sock >= 0) {
		close(sock);
		sock = -1;
	}
}

static void
setup_signals(void)
{
	struct sigaction sa = {
		.sa_sigaction= close_socket,
		.sa_flags = SA_SIGINFO,
		};
	sigemptyset(&sa.sa_mask);
	sigfillset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

int last = -1;

static int
__attribute__((unused))
csr(uint8_t *c)
{
	uint8_t buf[1];
	if (sock == -1)
		return -1;
	int rc;
	int retries = 5;
	while (1) {
		errno = 0;
		rc = read(sock, buf, 1);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN) {
				if (retries) {
					retries--;
					continue;
				}
				last = -1;
				errno = EPIPE;
				raise(SIGPIPE);
				return -errno;
			}
			err(6, "read()");
		}
		if (rc == 0) {
			last = -1;
			errno = EPIPE;
			raise(SIGPIPE);
			return -errno;
		}

		retries = 5;
		break;
	}
	if (rc > 0) {
		last = 0;
		*c = buf[0];
	}
	return rc;
}

static int
__attribute__((unused))
csw(uint8_t byte)
{
	if (sock == -1)
		return -1;

	int rc = write(sock, &byte, 1);
	if (rc < 0) {
		if (errno == EPIPE) {
			last = -1;
			errno = EPIPE;
			raise(SIGPIPE);
			return -errno;
		}
		err(8, "write()");
	}
	if (rc > 0)
		last = 1;
	return rc;
}

static int
__attribute__((unused))
read_byte(uint8_t *c)
{
	int rc = read(STDIN_FILENO, c, 1);
	if (rc < 0)
		return -errno;
	return rc;
}

static int
__attribute__((unused))
write_byte(uint8_t b)
{
	int rc = write(STDOUT_FILENO, &b, 1);
	if (rc < 0)
		return -errno;
	return rc;
}

void lock_buffer(int locked)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	if (locked)
		sigprocmask(SIG_BLOCK, &set, NULL);
	else
		sigprocmask(SIG_UNBLOCK, &set, NULL);
}

int
main(void)
{
	setup_signals();
	cooper_size_t read_pos;
	cooper_size_t write_pos;
	char buffer[4096];
	size_t size = 4096;
	int rc;

	rc = cooper_init();
	if (rc < 0)
		err(4, "cooper_init()");

	cooper_set_serial_buffer(buffer, size, &read_pos, &write_pos,
				 lock_buffer);

#ifdef COOPER_NETWORK
	int sd = set_up_socket();
	struct sockaddr addr;
	socklen_t len = sizeof (addr);

	printf("pid: %d\n", getpid());
#endif
	while (1) {
		struct cooper_timespec si = { 0, 200000000 };
#ifdef COOPER_NETWORK
		sock = accept(sd, &addr, &len);
		if (sock < 0) {
			if (errno == EINTR)
				continue;
			err(9, "accept()");
		}

		rc = fcntl(sock, F_GETFL);
		rc |= O_NONBLOCK;
		fcntl(sock, F_SETFL, rc);

		cooper_set_serial(csr, csw, si);
#else
		cooper_set_serial(read_byte, write_byte, si);
#endif
		cooper_set_timer_cb(set_timer);
		cooper_start_debugging();

		struct timespec ts = { 1, 0 };
		rc = 0;
		while (sock >= 0)
			nanosleep(&ts, NULL);
		cooper_stop_debugging();
	}

	return 0;
}

