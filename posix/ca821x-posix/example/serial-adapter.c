#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix.h"

#ifdef OPENTHREAD_TARGET_LINUX
#include <sys/prctl.h>
int   posix_openpt(int oflag);
int   grantpt(int fildes);
int   unlockpt(int fd);
char *ptsname(int fd);
#endif // OPENTHREAD_TARGET_LINUX

static struct ca821x_dev  sDeviceRef;
static struct ca821x_dev *pDeviceRef;

static uint8_t        s_receive_buffer[128];
static const uint8_t *s_write_buffer;
static size_t         s_write_length;
static int            s_in_fd;
static int            s_out_fd;

static uint8_t s_enabled = false;

static struct termios original_stdin_termios;
static struct termios original_stdout_termios;

static pthread_mutex_t s_write_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_write_cond  = PTHREAD_COND_INITIALIZER;

static void restore_stdin_termios(void)
{
	tcsetattr(s_in_fd, TCSAFLUSH, &original_stdin_termios);
}

static void restore_stdout_termios(void)
{
	tcsetattr(s_out_fd, TCSAFLUSH, &original_stdout_termios);
}

ca_error configure_io(void)
{
	ca_error       error = CA_ERROR_SUCCESS;
	struct termios termios;

	if (s_enabled)
	{
		return CA_ERROR_INVALID_STATE;
	}

#ifdef OPENTHREAD_TARGET_LINUX
	// Ensure we terminate this process if the
	// parent process dies.
	prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

	s_in_fd  = dup(STDIN_FILENO);
	s_out_fd = dup(STDOUT_FILENO);
	//dup2(STDERR_FILENO, STDOUT_FILENO);

	// We need this signal to make sure that this
	// process terminates properly.
	signal(SIGPIPE, SIG_DFL);

	if (isatty(s_in_fd))
	{
		tcgetattr(s_in_fd, &original_stdin_termios);
		atexit(&restore_stdin_termios);
	}

	if (isatty(s_out_fd))
	{
		tcgetattr(s_out_fd, &original_stdout_termios);
		atexit(&restore_stdout_termios);
	}

	if (isatty(s_in_fd))
	{
		// get current configuration
		if (tcgetattr(s_in_fd, &termios))
		{
			perror("tcgetattr");
			error = CA_ERROR_FAIL;
			goto exit;
		}

		// Set up the termios settings for raw mode. This turns
		// off input/output processing, line processing, and character processing.
		cfmakeraw(&termios);

		// Set up our cflags for local use. Turn on hangup-on-close.
		termios.c_cflag |= HUPCL | CREAD | CLOCAL;

		//enable terminal interrupts
		termios.c_lflag |= (ISIG);

		// "Minimum number of characters for noncanonical read"
		termios.c_cc[VMIN] = 1;

		// "Timeout in deciseconds for noncanonical read"
		termios.c_cc[VTIME] = 0;

		// configure baud rate
		if (cfsetispeed(&termios, B115200))
		{
			perror("cfsetispeed");
			error = CA_ERROR_FAIL;
		}

		// set configuration
		if (tcsetattr(s_in_fd, TCSANOW, &termios))
		{
			perror("tcsetattr");
			error = CA_ERROR_FAIL;
		}
	}

	if (isatty(s_out_fd))
	{
		// get current configuration
		if (tcgetattr(s_out_fd, &termios))
		{
			perror("tcgetattr");
			error = CA_ERROR_FAIL;
		}

		// Set up the termios settings for raw mode. This turns
		// off input/output processing, line processing, and character processing.
		cfmakeraw(&termios);

		// Absolutely obliterate all output processing.
		termios.c_oflag = 0;

		//enable terminal interrupts
		termios.c_lflag |= (ISIG);

		// Set up our cflags for local use. Turn on hangup-on-close.
		termios.c_cflag |= HUPCL | CREAD | CLOCAL;

		// configure baud rate
		if (cfsetospeed(&termios, B115200))
		{
			perror("cfsetospeed");
			error = CA_ERROR_FAIL;
		}

		// set configuration
		if (tcsetattr(s_out_fd, TCSANOW, &termios))
		{
			perror("tcsetattr");
			error = CA_ERROR_FAIL;
		}
	}

	if (error == CA_ERROR_SUCCESS)
		s_enabled = true;
	return error;

exit:
	close(s_in_fd);
	close(s_out_fd);
	if (error == CA_ERROR_SUCCESS)
		s_enabled = true;
	return error;
}

int handle_user_command(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	ca_error error = CA_ERROR_SUCCESS;

	if (buf[0] != 0xB3)
		return CA_ERROR_NOT_HANDLED;

	pthread_mutex_lock(&s_write_mutex);

	while (s_write_length) pthread_cond_wait(&s_write_cond, &s_write_mutex);

	s_write_buffer = buf + 2;
	s_write_length = len - 2;

	pthread_mutex_unlock(&s_write_mutex);

	return error;
}

void process_io(void)
{
	ssize_t       rval;
	const int     error_flags = POLLERR | POLLNVAL | POLLHUP;
	struct pollfd pollfd[]    = {
        {s_in_fd, POLLIN | error_flags, 0},
        {s_out_fd, POLLOUT | error_flags, 0},
    };

	errno = 0;

	rval = poll(pollfd, sizeof(pollfd) / sizeof(*pollfd), 0);

	if (rval < 0)
	{
		perror("poll");
		exit(EXIT_FAILURE);
	}

	if (rval > 0)
	{
		if ((pollfd[0].revents & error_flags) != 0)
		{
			perror("s_in_fd");
			exit(EXIT_FAILURE);
		}

		if ((pollfd[1].revents & error_flags) != 0)
		{
			perror("s_out_fd");
			exit(EXIT_FAILURE);
		}

		if (pollfd[0].revents & POLLIN)
		{
			rval = read(s_in_fd, s_receive_buffer, sizeof(s_receive_buffer));
			assert(sizeof(s_receive_buffer) < 255);

			if (rval <= 0)
			{
				perror("read");
				exit(EXIT_FAILURE);
			}

			// Send read message to connected device
			exchange_user_command(0xB2, (uint8_t)rval, s_receive_buffer, pDeviceRef);
		}

		pthread_mutex_lock(&s_write_mutex);
		if ((s_write_length > 0) && (pollfd[1].revents & POLLOUT))
		{
			rval = write(s_out_fd, s_write_buffer, s_write_length);

			if (rval <= 0)
			{
				perror("write");
				exit(EXIT_FAILURE);
			}

			s_write_buffer += rval;
			s_write_length -= rval;

			if (s_write_length == 0)
			{
				// Allow next message to be loaded;
				pthread_cond_signal(&s_write_cond);
			}
		}
		pthread_mutex_unlock(&s_write_mutex);
	}
}

int main(int argc, const char *argv[])
{
	pDeviceRef = &sDeviceRef;
	while (ca821x_util_init(pDeviceRef, NULL))
	{
		return -1;
		sleep(1);
	}
	configure_io();
	exchange_register_user_callback(handle_user_command, pDeviceRef);

	while (1)
	{
		process_io();
	}
}
