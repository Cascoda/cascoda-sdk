#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if __linux__
#include <string.h>
#include <time.h>
#endif // __linux__

#if defined(_WIN32)
#include <Windows.h>
#endif // _WIN32

#include "ca821x-posix/ca821x-posix.h"

struct ca821x_dev sDeviceRef;
uint32_t          numberOfMessagesSent;
uint32_t          numberOfIndicationsReceived = 0;
uint32_t          mReceivedIndications[300];
uint32_t          position       = 0;
uint8_t           previousHandle = 0;
uint8_t           count          = 0;
int               missedHandles  = 0;
char *            resultsOption;

#if __linux__
struct timespec *start;
struct timespec *end;
// __linux__
#elif defined(_WIN32)
LARGE_INTEGER  frequency;
LARGE_INTEGER *start;
LARGE_INTEGER *end;
#endif // _WIN32

static void displayHelp()
{
	printf("\n===== How to use the command =====\n");
	printf("\nThere are 2 ways to type the command:\n\n");

#if __linux__
	printf("1. usb_test ");
// __linux__
#elif defined(_WIN32)
	printf("1. usb_test.exe ");
#endif // _WIN32
	printf("<number of messages to send> (+ optional: <mode>) - The mode can be 'raw', 'statistics' or 'both'\n");
	printf("(note: if not specified, the mode is 'statistics' by default.)\n\n");

#if __linux__
	printf("2. usb_test ");
// __linux__
#elif defined(_WIN32)
	printf("2. usb_test.exe ");
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
#if __linux__
	if (clock_gettime(CLOCK_MONOTONIC, start + index) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
// __linux__
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
#if __linux__
	if (clock_gettime(CLOCK_MONOTONIC, end + position) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
// __linux__
#elif defined(_WIN32)
	if (QueryPerformanceCounter(end + position) == 0)
	{
		perror("QueryPerformanceTimer");
		exit(EXIT_FAILURE);
	}
#endif // _WIN32
}

ca_error handleUserCallback(const uint8_t *buf, size_t len, struct ca821x_dev *pDeviceRef)
{
	if (buf[0] == 0xA0)
	{
		if (strstr((char *)(buf + 2), "dispatching on SPI") != NULL)
		{
			return CA_ERROR_SUCCESS;
		}

		fprintf(stderr, "IN: %.*s\r\n", (int)(len - 2), buf + 2);
		return CA_ERROR_SUCCESS;
	}
	else if (buf[0] == 0xA2)
	{
		uint8_t handle = buf[2];
		uint8_t handlesDifference;

		if (previousHandle != handle)
		{
			count             = 0;
			handlesDifference = handle - previousHandle;

			position += handlesDifference;

			previousHandle = handle;
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
	return CA_ERROR_SUCCESS;
}

#if __linux__
static double timeSpecToMilliseconds(struct timespec *ts)
{
	return (double)ts->tv_sec * 1000.0 + (double)ts->tv_nsec / 1000000.0;
}
#endif //__linux__

static void fillElapsedTimeArray(double *timeArray, int index)
{
#if __linux__
	if (timeSpecToMilliseconds(end + index) < timeSpecToMilliseconds(start + index))
	{
		missedHandles++;
		*(timeArray + index) = 0.0;
	}
	else
	{
		*(timeArray + index) = timeSpecToMilliseconds(end + index) - timeSpecToMilliseconds(start + index);
	}
// __linux__
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

struct EVBME_COMM_CHECK_request
{
	uint8_t mHandle;   //!< Handle identifying this comm check
	uint8_t mDelay;    //!< Delay before sending responses
	uint8_t mIndCount; //!< Number of indications to send up
	uint8_t mIndSize;  //!< Size of the indications to send
	uint8_t mPayload[100];
};

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

int main(int argc, char *argv[])
{
	struct ca821x_dev *             pDeviceRef   = &(sDeviceRef);
	struct EVBME_COMM_CHECK_request CommCheckReq = {};
	uint32_t                        posixWait;
	uint32_t                        previousNumberOfIndications;
	uint32_t                        expectedNumberOfIndications;
	int                             waitForResult = 0;
	size_t                          baseSize      = sizeof(CommCheckReq) - sizeof(CommCheckReq.mPayload);
	size_t                          maxPayload    = sizeof(CommCheckReq.mPayload);
	size_t                          additionalLength;

	if (argc < 2)
	{
		printf("Too few arguments\n");
		displayHelp();
		exit(EXIT_FAILURE);
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

	printf("Initialising.");
	while (ca821x_util_init(pDeviceRef, NULL))
	{
		sleep(1); //Wait while there isn't a device available to connect
		printf(".");
	}
	//Register callbacks for async messages
	exchange_register_user_callback(&handleUserCallback, pDeviceRef);
	ca821x_util_start_downstream_dispatch_worker();

	MLME_RESET_request_sync(1, pDeviceRef);

	printf("\r\nInitialised.\r\n\n");

	// Set the parameters of the test
	if (argc > 3)
	{
		CommCheckReq.mDelay    = atoi(argv[2]);
		CommCheckReq.mIndCount = atoi(argv[3]);
		posixWait              = 1000 * atoi(argv[4]);

		additionalLength = atoi(argv[5]);
		if (additionalLength > 100)
		{
			printf("Maximum allowed additional length is 100 bytes.\n");
			exit(EXIT_FAILURE);
		}

		CommCheckReq.mIndSize = atoi(argv[6]);
		if (argc > 7)
			resultsOption = argv[7];
		else
			resultsOption = "statistics";
	}
	else
	{
		CommCheckReq.mDelay    = 10;
		CommCheckReq.mIndCount = 10;
		posixWait              = 1000 * 10;
		additionalLength       = 0;
		CommCheckReq.mIndSize  = 1;
		if (argc == 3)
			resultsOption = argv[2];
		else
			resultsOption = "statistics";
	}

	expectedNumberOfIndications = numberOfMessagesSent * CommCheckReq.mIndCount;

	printf("Testing with parameters: \n");
	printf("\t%-56s%u \n", "Number of messages:", numberOfMessagesSent);
	printf("\t%-56s%u \n", "Delay before indications (in ms):", CommCheckReq.mDelay);
	printf("\t%-56s%u \n", "Number of indications:", CommCheckReq.mIndCount);
	printf("\t%-56s%u \n", "Delay between each message sent (in ms):", posixWait / 1000);
	printf("\t%-56s%u \n", "Additional length to the 1-byte message (in bytes):", additionalLength);
	printf("\t%-56s%u \n", "Size of each indication (in bytes):", CommCheckReq.mIndSize);

	// Send the messages
	for (uint32_t i = 0; i < numberOfMessagesSent; i++)
	{
		CommCheckReq.mHandle = (uint8_t)i;
		usleep(posixWait);
		exchange_user_command(0xA1, baseSize + additionalLength, (uint8_t *)(&CommCheckReq), pDeviceRef);
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
			displayResults(expectedNumberOfIndications, &CommCheckReq);
			break;
		}
	}
	return 0;
}
