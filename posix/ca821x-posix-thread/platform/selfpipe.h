/*
 * selfpipe.h
 *
 *  Created on: 23 Aug 2016
 *      Author: ciaran
 */

#ifndef PLATFORM_SELFPIPE_H_
#define PLATFORM_SELFPIPE_H_

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

void selfpipe_init(void);

void selfpipe_push(void);
void selfpipe_pop(void);

void selfpipe_UpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_SELFPIPE_H_ */
