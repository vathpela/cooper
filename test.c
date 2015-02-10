
#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "include/api.h"

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
	int timer_number;
	void *priv;
};

static struct timer_data sigdata = { NULL, 0, NULL };

int timer_fired = 0;

static void timer_dispatcher(int signal, siginfo_t *siginfo, void *ucontext)
{
	timer_fired = 1;
	sigdata.cb(sigdata.timer_number, sigdata.priv);
}

static int
set_timer(cooper_timer_cb cb,
	  struct cooper_timespec frequency,
	  void *cbdata)
{
	struct sigaction sa = {
		.sa_sigaction= timer_dispatcher,
		.sa_flags = SA_SIGINFO,
	};
	struct itimerval itimer = {
		.it_interval = { 0, 200000 },
		.it_value = { 0, 200000 }
		};

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaction(SIGALRM, &sa, NULL);

	setitimer(ITIMER_REAL, &itimer, NULL);

	sigdata.cb = cb;
	sigdata.priv = cbdata;
	return 0;
}

int sock = -1;

static uint8_t
__attribute__((unused))
csr(void)
{
	uint8_t buf[1];
	if (sock == -1)
		err(5, "wtf");
	int rc;
	rc = read(sock, buf, 1);
	if (rc < 0)
		err(6, "read()");
	printf("r: %x (%c)\n", buf[0], buf[0]);
	return buf[0];
}

static void
__attribute__((unused))
csw(uint8_t byte)
{
	if (sock == -1)
		err(7, "wtf");

	int rc = write(sock, &byte, 1);
	if (rc < 0)
		err(8, "write()");
}

static uint8_t
__attribute__((unused))
read_byte(void)
{
	char buf;
	read(STDIN_FILENO, &buf, 1);
	return buf;
}

static void
__attribute__((unused))
write_byte(uint8_t b)
{
	write(STDOUT_FILENO, &b, 1);
}

int
main(void)
{
#if 1
	int rc;

	rc = cooper_init();
	if (rc < 0)
		err(4, "cooper_init()");

	int sd = set_up_socket();
	struct sockaddr addr;
	socklen_t len = sizeof (addr);

	printf("pid: %d\n", getpid());
	sock = accept(sd, &addr, &len);
	if (sock < 0)
		err(9, "accept()");
	printf("sock: %d\n", sock);

	rc = fcntl(sock, F_GETFL);
	rc |= O_NONBLOCK;
	fcntl(sock, F_SETFL, rc);

	struct cooper_timespec si = { 0, 200000 };
	cooper_set_serial(csr, csw, si);
#else
	struct cooper_timespec si = { 0, 200000 };
	cooper_set_serial(read_byte, write_byte, si);
#endif
	cooper_set_timer_cb(set_timer);
	cooper_start_debugging();

	while (1) {
		struct timespec ts = { 1, 0};
		nanosleep(&ts, NULL);
		//printf("nanosleep ended %s timer firing\n", timer_fired ? "with" : "without");
		timer_fired = 0;
	}

	return 0;
}

