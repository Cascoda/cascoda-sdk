#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "selfpipe.h"

static int fd[2];

void selfpipe_init(void)
{
	pipe2(fd, O_NONBLOCK);
}

void selfpipe_push(void)
{
	write(fd[1], "a", 1);
}

void selfpipe_pop(void)
{
	uint8_t junkBuf;
	read(fd[0], &junkBuf, 1);
}

void selfpipe_UpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
	if (aReadFdSet != NULL)
	{
		FD_SET(fd[0], aReadFdSet);

		if (aMaxFd != NULL && *aMaxFd < fd[0])
		{
			*aMaxFd = fd[0];
		}
	}
}
