/*
 * Copyright (c) 2020, Cascoda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifndef _WIN32
#include <string.h>
#endif // POSIX

#if defined(_WIN32)
#include <Windows.h>
#endif // _WIN32

#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x-posix/ca821x-posix.h"
#include "evbme_messages.h"

struct ca821x_dev sDeviceRef;
uint32_t          numberOfMessagesSent;
uint32_t          numberOfIndicationsReceived = 0;
uint32_t          mReceivedIndications[300];
uint32_t          position       = 0;
uint8_t           previousHandle = 0;
uint8_t           count          = 0;
int               missedHandles  = 0;
char *            resultsOption;

#ifndef _WIN32
struct timespec *start;
struct timespec *end;
// POSIX
#elif defined(_WIN32)
LARGE_INTEGER  frequency;
LARGE_INTEGER *start;
LARGE_INTEGER *end;
#endif // _WIN32

static void displayHelp()
{
	printf("\n===== How to use the command =====\n");
	printf("\nThere are 3 ways to type the command:\n\n");

#ifndef _WIN32
	printf("1. serial-test\n");
// POSIX
#elif defined(_WIN32)
	printf("1. serial-test.exe\n");
#endif // _WIN32
	printf("(note: if no arguments, the USB connection is held open, but no messages are sent. Useful for intercepting "
	       "EVBME messages only.)\n\n");

#ifndef _WIN32
	printf("2. serial-test ");
// POSIX
#elif defined(_WIN32)
	printf("2. serial-test.exe ");
#endif // _WIN32
	printf("<number of messages to send> (+ optional: <mode>) - The mode can be 'raw', 'statistics' or 'both'\n");
	printf("(note: if not specified, the mode is 'statistics' by default.)\n\n");

#ifndef _WIN32
	printf("3. serial-test ");
// POSIX
#elif defined(_WIN32)
	printf("3. serial-test.exe ");
#endif // _WIN32
	printf("<arg1> <arg2> <arg3> <arg4> <arg5> <arg6> (+ optional: <mode>), where:\n");
	printf("\t<arg1> - Number of messages to send to the Chili module.\n");
	printf("\t<arg2> - Delay (in ms) before the Chili module sends indications.\n");
	printf("\t<arg3> - Number of indications to be sent by the Chili module.\n");
	printf("\t<arg4> - Delay (in ms) between each message sent to the Chili module.\n");
	printf("\t<arg5> - Additional length (in bytes) to add the default 1-byte message sent to the Chili module.\n");
	printf("\t<arg6> - Length (in bytes) of each indication sent by the Chili module.\n\n");
}

static void getStartTime(int index)
{
#ifndef _WIN32
	if (clock_gettime(CLOCK_MONOTONIC, start + index) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
// POSIX
#elif defined(_WIN32)
	if (QueryPerformanceCounter(start + index) == 0)
	{
		perror("QueryPerformanceTimer");
		exit(EXIT_FAILURE);
	}
#endif // _WIN32
}

static void getEndTime()
{
#ifndef _WIN32
	if (clock_gettime(CLOCK_MONOTONIC, end + position) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
// POSIX
#elif defined(_WIN32)
	if (QueryPerformanceCounter(end + position) == 0)
	{
		perror("QueryPerformanceTimer");
		exit(EXIT_FAILURE);
	}
#endif // _WIN32
}

static ca_error handleEvbmeMessage(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	fprintf(stderr, "IN: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

static ca_error handleCommIndication(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	struct EVBME_COMM_indication *commInd = &(params->EVBME.COMM_indication);
	uint8_t                       handlesDifference;

	if (previousHandle != commInd->mHandle)
	{
		count             = 0;
		handlesDifference = commInd->mHandle - previousHandle;

		position += handlesDifference;

		previousHandle = commInd->mHandle;
	}
	if (count == 0)
	{
		getEndTime();
	}
	numberOfIndicationsReceived++;
	mReceivedIndications[count]++;
	count++;

	return CA_ERROR_SUCCESS;
}

#ifndef _WIN32
static double timeSpecToMilliseconds(struct timespec *ts)
{
	return (double)ts->tv_sec * 1000.0 + (double)ts->tv_nsec / 1000000.0;
}
#endif //__linux__

static void fillElapsedTimeArray(double *timeArray, int index)
{
#ifndef _WIN32
	if (timeSpecToMilliseconds(end + index) < timeSpecToMilliseconds(start + index))
	{
		missedHandles++;
		*(timeArray + index) = 0.0;
	}
	else
	{
		*(timeArray + index) = timeSpecToMilliseconds(end + index) - timeSpecToMilliseconds(start + index);
	}
// POSIX
#elif defined(_WIN32)
	if ((end + index)->QuadPart < (start + index)->QuadPart)
	{
		missedHandles++;
		*(timeArray + index) = 0.0;
	}
	else
	{
		*(timeArray + index) =
		    1000 * ((end + index)->QuadPart - (start + index)->QuadPart) / (double)frequency.QuadPart;
	}
#endif // _WIN32
}

static void resultsAnalysis()
{
	double *elapsedMilliseconds, *elapsedMillisecondsReduced;
	double  min, max, mean, median, standardDeviation = 0;
	bool    arrayFilled = false;

	elapsedMilliseconds = malloc(sizeof(*elapsedMilliseconds) * numberOfMessagesSent);

	printf("\n");

	if (strcmp(resultsOption, "raw") == 0 || strcmp(resultsOption, "both") == 0)
	{
		printf("Time between message sent and first indication received:\n");
		for (int i = 0; i < numberOfMessagesSent; i++)
		{
			fillElapsedTimeArray(elapsedMilliseconds, i);
			arrayFilled = true;

			if (elapsedMilliseconds[i] < 0.01)
				printf("\tmessage %d: No indications received\n", i);
			else
				printf("\tmessage %d: %f ms\n", i, elapsedMilliseconds[i]);
		}
		printf("\n");

		if (numberOfMessagesSent == missedHandles)
			return;
	}

	if (strcmp(resultsOption, "statistics") == 0 || strcmp(resultsOption, "both") == 0)
	{
		min  = 10000.0;
		max  = 0.1;
		mean = 0.0;

		for (int i = 0; i < numberOfMessagesSent; i++)
		{
			if (!arrayFilled)
				fillElapsedTimeArray(elapsedMilliseconds, i);

			mean += elapsedMilliseconds[i];

			if (elapsedMilliseconds[i] < min && elapsedMilliseconds[i] > 0.1)
				min = elapsedMilliseconds[i];
			if (elapsedMilliseconds[i] > max)
				max = elapsedMilliseconds[i];
		}

		if (numberOfMessagesSent == missedHandles)
			return;

		printf("Summary statistics about elapsed time between each message and its first response:\n");

		mean /= (numberOfMessagesSent - missedHandles);

		// Determine/calculate the median.
		if (missedHandles > 0)
		{
			int index                  = 0;
			elapsedMillisecondsReduced = malloc(sizeof(*elapsedMilliseconds) * (numberOfMessagesSent - missedHandles));

			for (int i = 0; i < numberOfMessagesSent; i++)
			{
				if (elapsedMilliseconds[i] > 0.1)
				{
					elapsedMillisecondsReduced[index] = elapsedMilliseconds[i];
					index++;
				}
			}

			if ((numberOfMessagesSent - missedHandles) % 2 == 0)
			{
				double firstNumber  = elapsedMillisecondsReduced[((numberOfMessagesSent - missedHandles) / 2) - 1];
				double secondNumber = elapsedMillisecondsReduced[(numberOfMessagesSent - missedHandles) / 2];
				median              = (firstNumber + secondNumber) / 2;
			}
			else
			{
				median = elapsedMillisecondsReduced[((numberOfMessagesSent - missedHandles) - 1) / 2];
			}
		}
		else
		{
			if (numberOfMessagesSent % 2 == 0)
			{
				double firstNumber  = elapsedMilliseconds[(numberOfMessagesSent / 2) - 1];
				double secondNumber = elapsedMilliseconds[numberOfMessagesSent / 2];
				median              = (firstNumber + secondNumber) / 2;
			}
			else
			{
				median = elapsedMilliseconds[(numberOfMessagesSent - 1) / 2];
			}
		}

		// Calculate the standard deviation
		if (missedHandles > 0)
		{
			for (int i = 0; i < (numberOfMessagesSent - missedHandles); i++)
			{
				standardDeviation += pow(elapsedMillisecondsReduced[i] - mean, 2);
			}
			standardDeviation /= (numberOfMessagesSent - missedHandles);
		}
		else
		{
			for (int i = 0; i < numberOfMessagesSent; i++)
			{
				standardDeviation += pow(elapsedMilliseconds[i] - mean, 2);
			}
			standardDeviation /= numberOfMessagesSent;
		}

		standardDeviation = sqrt(standardDeviation);

		// Display results
		printf("\t%-21s%11f ms \n\t%-21s%11f ms \n\t%-21s%11f ms \n\t%-21s%11f ms \n\t%-21s%11f ms\n\n",
		       "min:",
		       min,
		       "max:",
		       max,
		       "mean:",
		       mean,
		       "median:",
		       median,
		       "standard deviation:",
		       standardDeviation);
	}
}

static void displayResults(uint32_t expectedIndications, struct EVBME_COMM_CHECK_request *CommCheckReq)
{
	printf("\nResult: %d/%d indications received.", numberOfIndicationsReceived, expectedIndications);
	for (int i = 0; i < CommCheckReq->mIndCount; i++)
	{
		if (i % 25 == 0)
			printf("\n");

		printf("%5d ", mReceivedIndications[i]);
	}
	printf("\n");

	// Analyse the results
	resultsAnalysis();

	if (numberOfMessagesSent == missedHandles)
		printf("No indications were received\n");
	else if (missedHandles > 0)
		printf("No indication was received for %d messages\n", missedHandles);
}

// A simplified loop to purely connect to a device and display the evbme messages
static void displayEvbme(struct ca821x_dev *pDeviceRef)
{
	//Disable the comm indication checking
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_COMM_indication = NULL;

	while (1)
	{
		//Just sleep until the end of time and send occasional COMM_CHECKs, threads will handle everything else
		EVBME_COMM_CHECK_request(0, 0, 1, 5, 1, pDeviceRef);
		sleep(10);
	}
}

static void initialise_ca821x(struct ca821x_dev *pDeviceRef)
{
	printf("Initialising.");
	while (ca821x_util_init(pDeviceRef, NULL))
	{
		sleep(1); //Wait while there isn't a device available to connect
		printf(".");
	}
	//Register callbacks for async messages
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handleEvbmeMessage;
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_COMM_indication    = &handleCommIndication;
	ca821x_util_start_downstream_dispatch_worker();

	printf("\r\nInitialised.\r\n\n");
}

int main(int argc, char *argv[])
{
	struct ca821x_dev *             pDeviceRef = &(sDeviceRef);
	struct EVBME_COMM_CHECK_request commCheckReq;
	uint32_t                        posixWaitMs = 10;
	struct timespec                 posixWait   = {};
	uint32_t                        previousNumberOfIndications;
	uint32_t                        expectedNumberOfIndications;
	int                             waitForResult = 0;
	size_t                          additionalLength;

	if (argc < 2)
	{
		printf("Too few arguments for test - entering EVBME mode\n");
		displayHelp();
		initialise_ca821x(pDeviceRef);
		displayEvbme(pDeviceRef);
	}

	if (argc > 8)
	{
		printf("Too many arguments\n");
		displayHelp();
		exit(EXIT_FAILURE);
	}

#if defined(_WIN32)
	if (QueryPerformanceFrequency(&frequency) == 0)
	{
		perror("QueryPerformanceFrequency");
		exit(EXIT_FAILURE);
	}
#endif // _WIN32

	numberOfMessagesSent = atoi(argv[1]);

	start = malloc(sizeof(*start) * numberOfMessagesSent);
	end   = calloc(numberOfMessagesSent, sizeof(*end));

	initialise_ca821x(pDeviceRef);

	// Set the parameters of the test
	if (argc > 3)
	{
		commCheckReq.mDelay    = atoi(argv[2]);
		commCheckReq.mIndCount = atoi(argv[3]);
		posixWaitMs            = atoi(argv[4]);

		additionalLength = atoi(argv[5]);
		if (additionalLength > 100)
		{
			printf("Maximum allowed additional length is 100 bytes.\n");
			exit(EXIT_FAILURE);
		}

		commCheckReq.mIndSize = atoi(argv[6]);
		if (argc > 7)
			resultsOption = argv[7];
		else
			resultsOption = "statistics";
	}
	else
	{
		commCheckReq.mDelay    = 10;
		commCheckReq.mIndCount = 10;
		posixWaitMs            = 10;
		additionalLength       = 0;
		commCheckReq.mIndSize  = 1;
		if (argc == 3)
			resultsOption = argv[2];
		else
			resultsOption = "statistics";
	}

	expectedNumberOfIndications = numberOfMessagesSent * commCheckReq.mIndCount;

	posixWait.tv_sec  = posixWaitMs / 1000;
	posixWait.tv_nsec = (posixWaitMs % 1000) * 1000000;

	printf("Testing with parameters: \n");
	printf("\t%-56s%u \n", "Number of messages:", numberOfMessagesSent);
	printf("\t%-56s%u \n", "Delay before indications (in ms):", commCheckReq.mDelay);
	printf("\t%-56s%u \n", "Number of indications:", commCheckReq.mIndCount);
	printf("\t%-56s%u \n", "Delay between each message sent (in ms):", posixWaitMs);
	printf("\t%-56s%u \n", "Additional length to the 1-byte message (in bytes):", additionalLength);
	printf("\t%-56s%u \n", "Size of each indication (in bytes):", commCheckReq.mIndSize);

	// Send the messages
	for (uint32_t i = 0; i < numberOfMessagesSent; i++)
	{
		commCheckReq.mHandle = (uint8_t)i;
		nanosleep(&posixWait, NULL);
		EVBME_COMM_CHECK_request((uint8_t)i,
		                         commCheckReq.mDelay,
		                         commCheckReq.mIndCount,
		                         commCheckReq.mIndSize,
		                         additionalLength,
		                         pDeviceRef);
		getStartTime(i);
	}

	while (1)
	{
		previousNumberOfIndications = numberOfIndicationsReceived;

		sleep(1);

		if (numberOfIndicationsReceived > previousNumberOfIndications)
		{
			// If a new indication is received, keep waiting
			waitForResult = 0;
			continue;
		}

		waitForResult++;

		if ((expectedNumberOfIndications == numberOfIndicationsReceived) || waitForResult > 5)
		{
			displayResults(expectedNumberOfIndications, &commCheckReq);
			break;
		}
	}
	return 0;
}
