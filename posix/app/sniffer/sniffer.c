/*
 *  Copyright (c) 2019, Cascoda Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file
 * Sniffer implementation for capturing 802.15.4 packets on a given channel.
 */
#if defined(_WIN32)
#include <Windows.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ca821x-posix/ca821x-posix-evbme.h"
#include "ca821x-posix/ca821x-posix.h"
#include "evbme_messages.h"

#if CASCODA_CA_VER != 8210

#if _WIN32
#define DEFAULT_PIPE "\\\\.\\pipe\\cascoda_"
#else
#define DEFAULT_PIPE "/tmp/cascoda_"
#endif

/**
 * Ethernet header for encapsulating IEEE802.15.4 frames for pcap.
 */
uint8_t ethernet_header[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x80, 0x9a};

/**
 * Pcap file header struct as https://wiki.wireshark.org/Development/LibpcapFileFormat
 */
typedef struct pcap_hdr_s
{
	uint32_t magic_number;  ///< magic number
	uint16_t version_major; ///< major version number
	uint16_t version_minor; ///< minor version number
	int32_t  thiszone;      ///< GMT to local correction
	uint32_t sigfigs;       ///< accuracy of timestamps
	uint32_t snaplen;       ///< max length of captured packets, in octets
	uint32_t network;       ///< data link type
} pcap_hdr_t;

/**
 * Pcap packet header struct as https://wiki.wireshark.org/Development/LibpcapFileFormat
 */
typedef struct pcaprec_hdr_s
{
	uint32_t ts_sec;   ///< timestamp seconds
	uint32_t ts_usec;  ///< timestamp microseconds
	uint32_t incl_len; ///< number of octets of packet saved in file
	uint32_t orig_len; ///< actual length of packet
} pcaprec_hdr_t;

ca_static_assert(sizeof(pcap_hdr_t) == 24);    //pcap_hdr_t_not_packed
ca_static_assert(sizeof(pcaprec_hdr_t) == 16); //pcaprec_hdr_t_not_packed

/**
 * Output mode for formatting printed data
 */
enum
{
	OUT_MODE_HEX,  //!< Print in hex
	OUT_MODE_PCAP, //!< Print binary pcap format
} out_mode = OUT_MODE_HEX;

/** static cascoda device reference */
struct ca821x_dev sDeviceRef;

#if defined(_WIN32)
HANDLE        output;
LARGE_INTEGER start;
LARGE_INTEGER frequency;
#else  //posix
struct timespec start;
FILE           *output;
#endif // _WIN32

/** Enable logging of extra info to stderr in pcap mode */
bool debugMode = false;
/** Channel we are sniffing on */
uint8_t channel = 0;
/** Static storage for default pipe name */
char default_pipe[30];
/** Pointer to dynamic pipe name. */
char *dPipeName = NULL; //Dynamic memory

/**
 * Platform abstraction function to flush the output, causing data to be written now.
 */
static void ca_flush(void);

/**
 * Platform abstraction function to write a buffer to output.
 * @param buf Buffer to write
 * @param len Length of the buffer to write.
 * @return Error code
 * @retval CA_ERROR_SUCCESS  Successful write
 * @retval CA_ERROR_FAIL  Write failed.
 */
static ca_error ca_write(const void *buf, size_t len);

/**
 * Platform abstraction function to print a format string to the correct output.
 * Used like printf - format string followed by variable args.
 * @param format Format string
 */
static void ca_print(const char *format, ...);

/**
 * Platform abstraction function to clean up pipe on program termination.
 */
static void clean_pipe();

/**
 * Platform abstraction function to create a system pipe.
 */
static void create_pipe(char *name);

/**
 * Platform abstraction function to open a system pipe for writing.
 */
static void open_pipe(char *name);

/**
 * Platform abstraction function to get the default wireshark executable pipe.
 */
static char *getDefaultWiresharkPath(void);

/**
 * Platform abstraction function to start wireshark and bind it to this program.
 */
static void start_wireshark(const char *wspath);

/**
 * Platform abstraction function to register the start time of the program.
 */
static void setStartTime(void);

/**
 * Platform abstraction function to fill in the timing information for the pcap header.
 * @param pcapHeader A pointer to the pcap header to be filled.
 * @param params A pointer to the received PCPS indication
 */
static void fillTimestamp(pcaprec_hdr_t *pcapHeader, struct PCPS_DATA_indication_pset *params);

/**
 * Platform abstraction function to configure the default system output.
 */
static void configure_io(void);

/**
 * Platform abstraction function to disable all output processing.
 */
static void io_raw(void);

/**
 * Helper function to print the help information to stderr.
 */
static void displayHelp()
{
	fprintf(stderr, "\n===== How to use the command =====\n");

#if defined(_WIN32)
	fprintf(stderr, "sniffer.exe ");
#else  //posix
    fprintf(stderr, "sniffer ");
#endif // _WIN32
	fprintf(stderr, "[OPTIONS] CHANNEL\n");
	fprintf(stderr, "\tPrint all received packets on channel (11-26)\n\n");
	fprintf(stderr, "DESCRIPTION\n");
	fprintf(stderr, "\tSniffer program to use the CA-8211 to capture packets on a channel.\n\n");
	fprintf(stderr, "\t-p             PCap mode, output pcap data instead of descriptive hex\n");
	fprintf(stderr, "\t               dump. If this is used in conjunction with pipes, can be\n");
	fprintf(stderr, "\t               used to stream to wireshark\n\n");
	fprintf(stderr, "\t-d             Debug mode, print verbose information to stderr.\n\n");
	fprintf(stderr, "\t-n [PIPENAME]  Output to a named pipe/fifo, which can be read by\n");
	fprintf(stderr, "\t               wireshark or another program. Most useful in conjunction\n");
	fprintf(stderr, "\t               with '-p'.\n\n");
	fprintf(stderr, "\t-w             Open WireShark to process the packet capture. Implies -p\n");
	fprintf(stderr, "\t               and -n (random name for pipe if not provided separately).\n\n");
	fprintf(stderr, "\t-W [PATH]      Open WireShark at the path to process the packet capture.\n");
	fprintf(stderr, "\t               Implies -w.\n\n");
}

#if defined(_WIN32) //Windows abstraction
static void ca_flush(void)
{
	FlushFileBuffers(output);
}

static ca_error ca_write(const void *buf, size_t len)
{
	DWORD    bytesWritten = 0;
	ca_error error        = CA_ERROR_SUCCESS;

	WriteFile(output, buf, len, &bytesWritten, NULL);
	if (bytesWritten != len)
	{
		if (debugMode)
			fprintf(stderr, "Failed write.\n");
		error = CA_ERROR_FAIL;
	}
	return error;
}

static void ca_print(const char *format, ...)
{
	va_list     va_args;
	static char buffer[1000];
	int         len;
	va_start(va_args, format);
	len = vsnprintf(buffer, sizeof(buffer), format, va_args);
	if (len < sizeof(buffer))
		ca_write(buffer, len);
	va_end(va_args);
}

static void clean_pipe()
{
	FlushFileBuffers(output);
	DisconnectNamedPipe(output);
	CloseHandle(output);
}

static void create_pipe(char *name)
{
	char        prefix[]  = "\\\\.\\pipe\\";
	static bool atExitReg = false;

	if (memcmp(prefix, name, sizeof(prefix) - 1))
	{
		//No pipe prefix, add this
		size_t pipelen = sizeof(prefix) + strlen(name);
		dPipeName      = malloc(pipelen); //Freed at program termination
		memcpy(dPipeName, prefix, sizeof(prefix));
		strcat(dPipeName, name);
	}
	else
	{
		dPipeName = malloc(strlen(name) + 1);
		strcpy(dPipeName, name);
	}

	output =
	    CreateNamedPipe(dPipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 65536, 65536, 300, NULL);
	if (output == INVALID_HANDLE_VALUE)
	{
		perror("CreateNamedPipe");
		goto exit;
	}

	if (!atExitReg)
	{
		atexit(clean_pipe);
		atExitReg = true;
	}

exit:
	return;
}

static void open_pipe(char *name)
{
	DisconnectNamedPipe(output);
	if (!ConnectNamedPipe(output, NULL))
		fprintf(stderr, "Failed to connect to pipe as server.\n");
}

static char *getDefaultWiresharkPath(void)
{
	char        progPath[] = "\\Wireshark\\Wireshark.exe";
	bool        success    = false;
	static char FullPath[MAX_PATH];

	memset(FullPath, 0, sizeof(FullPath));
	if (GetEnvironmentVariable("programfiles", FullPath, MAX_PATH))
	{
		strcat(FullPath, progPath);
		success = true;
	}

	return success ? FullPath : NULL;
}

static void start_wireshark(const char *wspath)
{
	int                 rval;
	int                 arglen = strlen(dPipeName) + 20;
	char                args[arglen];
	STARTUPINFO         StartupInfo;
	PROCESS_INFORMATION ProcessInfo;

	memset(&StartupInfo, 0, sizeof(StartupInfo));
	memset(&ProcessInfo, 0, sizeof(ProcessInfo));
	snprintf(args, arglen, " -i %s -k", dPipeName);

	fprintf(stderr, "Starting wireshark...\n");
	rval = CreateProcess(wspath, args, NULL, NULL, false, 0, NULL, NULL, &StartupInfo, &ProcessInfo);

	if (!rval)
		fprintf(stderr, "Failed to start Wireshark, check path.\n");
}

static void setStartTime(void)
{
	if (QueryPerformanceFrequency(&frequency) == 0)
	{
		perror("QueryPerformanceFrequency");
		exit(EXIT_FAILURE);
	}
	if (QueryPerformanceCounter(&start) == 0)
	{
		perror("QueryPerformanceTimer");
		exit(EXIT_FAILURE);
	}
}

#if CASCODA_CA_VER <= 8211
static void fillTimestamp(pcaprec_hdr_t *pcapHeader, struct PCPS_DATA_indication_pset *params)
{
	LARGE_INTEGER now;
	(void)params;
	if (QueryPerformanceCounter(&now) == 0)
	{
		perror("QueryPerformanceTimer");
		exit(EXIT_FAILURE);
	}
	now.QuadPart       = now.QuadPart - start.QuadPart;
	pcapHeader->ts_sec = now.QuadPart / frequency.QuadPart;
	//Now calculate microseconds
	now.QuadPart *= 1000000;
	now.QuadPart /= frequency.QuadPart;
	now.QuadPart -= pcapHeader->ts_sec * 1000000;
	pcapHeader->ts_usec = (uint32_t)now.QuadPart;
}
#endif // CASCODA_CA_VER <= 8211

static void configure_io(void)
{
	output = GetStdHandle(STD_OUTPUT_HANDLE);
}

static void io_raw(void)
{
	setmode(STDOUT_FILENO, O_BINARY);
}
//End of windows abstraction
#else //posix abstraction
/**
 * Subtract one timespec from another.
 * @param t1 timespec 1
 * @param t2 timespec 2
 * @return The difference between t1 and t2
 */
static struct timespec timeSub(const struct timespec *t1, const struct timespec *t2)
{
	struct timespec curTime = {t1->tv_sec, t1->tv_nsec};
	//Get time difference
	curTime.tv_sec -= t2->tv_sec;
	curTime.tv_nsec -= t2->tv_nsec;

	// Normalize the struct in case the subtraction made negative nsec
	if (curTime.tv_nsec < 0)
	{
		curTime.tv_nsec += 1000000000L;
		curTime.tv_sec -= 1;
	}

	return curTime;
}

static void ca_flush(void)
{
	fflush(output);
}

static void ca_print(const char *format, ...)
{
	va_list va_args;
	va_start(va_args, format);
	vfprintf(output, format, va_args);
	va_end(va_args);
}

static ca_error ca_write(const void *buf, size_t len)
{
	write(fileno(output), buf, len);
	return CA_ERROR_SUCCESS;
}

static void clean_pipe(void)
{
	fclose(output);
	if (dPipeName)
		remove(dPipeName);
}

static void create_pipe(char *name)
{
	int         fd        = mkfifo(name, 0666);
	static bool atExitReg = false;
	if (fd < 0)
	{
		if (errno == EEXIST)
		{
			fprintf(stderr, "Warn: File %s already exists, continuing...\n", name);
		}
		else
		{
			perror("mkfifo");
			goto exit;
		}
	}
	dPipeName = malloc(strlen(name) + 1);
	strcpy(dPipeName, name);

	if (!atExitReg)
	{
		atexit(clean_pipe);
		atExitReg = true;
	}

exit:
	return;
}

static void open_pipe(char *name)
{
	output = fopen(dPipeName, "w");
	if (!output)
	{
		perror("fopen");
		return;
	}
}

static char *getDefaultWiresharkPath(void)
{
	return "wireshark";
}

static void start_wireshark(const char *wspath)
{
	int pid = fork();
	if (pid == 0)
	{
		int rval;
		// We are in the child process, execute the command
		int  cmdlen = strlen(wspath) + strlen(dPipeName) + 20;
		char buffer[cmdlen];
		snprintf(buffer, cmdlen, "%s -i %s -k", wspath, dPipeName);
		rval = system(buffer);
		if (rval < 0)
			fprintf(stderr, "Wireshark failed to start. Check path.\n");
		//Exit if process terminates.
		exit(0);
	}
}

static void setStartTime(void)
{
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
}

#if CASCODA_CA_VER <= 8211
static void fillTimestamp(pcaprec_hdr_t *pcapHeader, struct PCPS_DATA_indication_pset *params)
{
	struct timespec now;

	(void)params;
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
	now                 = timeSub(&now, &start);
	pcapHeader->ts_sec  = (uint32_t)(now.tv_sec);
	pcapHeader->ts_usec = (uint32_t)(now.tv_nsec / 1000);
}
#endif // CASCODA_CA_VER <= 8211

static void configure_io(void)
{
	output = stdout;
}

static void io_raw(void)
{
}
#endif //End of posix abstraction

#if CASCODA_CA_VER >= 8212
static void fillTimestamp(pcaprec_hdr_t *pcapHeader, struct PCPS_DATA_indication_pset *params)
{
	const uint64_t micros = 1000000;
	uint32_t       ts     = GETLE32(params->Timestamp); // timestamp in symbols
	uint64_t       ts_us  = (uint64_t)ts * aSymbolPeriod_us;
	uint64_t       ts_s   = ts_us / micros;

	pcapHeader->ts_sec  = (uint32_t)ts_s;
	pcapHeader->ts_usec = (uint32_t)(ts_us - (ts_s * micros));
}
#endif // CASCODA_CA_VER >= 8212

/**
 * Helper function to print the system time to the given output.
 * @param out  The FILE* to print to, or NULL to use default output.
 */
static void printTime(FILE *out)
{
	struct timespec ts;
	char            timeString[40];
	time_t          secs;

	clock_gettime(CLOCK_REALTIME, &ts);
	secs = ts.tv_sec;
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&secs));

	if (out)
		fprintf(out, "%s.%03d ", timeString, ts.tv_nsec / 1000000);
	else
		ca_print("%s.%03d ", timeString, ts.tv_nsec / 1000000);
}

/**
 * Print the pcap header to default output.
 */
static void printPcapHeader(void)
{
	pcap_hdr_t hdr = {
	    0xa1b2c3d4, //Magic pcap number
	    2,          //Major version
	    4,          //Minor Version
	    0,          //Timezone (dont care)
	    0,          //Timestamp precision (dont care)
	    300,        //Max packet size (good enough)
	    1           //Network = ethernet, used to encapsulate 802.15.4 frames
	};

	ca_write(&hdr, sizeof(hdr));
	ca_flush();
}

/**
 * Reset and initialise the radio for sniffing.
 * @param pDeviceRef cascoda device reference.
 */
static void initialiseRadio(struct ca821x_dev *pDeviceRef)
{
	EVBME_HOST_CONNECTED_notify(pDeviceRef); // reset device with hardware reset
	MLME_RESET_request_sync(1, pDeviceRef);
	uint8_t one = 1;
	//Set RxMode to PCPS
	HWME_SET_request_sync(HWME_RXMODE, 1, &one, pDeviceRef);
	//Set current channel to selected
	MLME_SET_request_sync(phyCurrentChannel, 0, 1, &channel, pDeviceRef);
	//Enable promiscuous mode, to receive all frames
	MLME_SET_request_sync(macPromiscuousMode, 0, 1, &one, pDeviceRef);
	//Enable receiving FCS field
	uint8_t macCfg = 0;
	TDME_GETSFR_request_sync(0, 0xD9, &macCfg, pDeviceRef);
	macCfg |= 0x10;
	TDME_SETSFR_request_sync(0, 0xD9, macCfg, pDeviceRef);
	//Set Rx enabled
	MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &one, pDeviceRef);
}

/**
 * Callback for evbme messages
 * @param params pointer to EVBME message containing the message indication.
 * @param pDeviceRef  cascoda device reference
 * @return CA_ERROR_SUCCESS
 */
static ca_error handleEvbmeMessage(struct EVBME_Message *params, struct ca821x_dev *pDeviceRef)
{
	fprintf(stderr, "IN: %.*s\r\n", params->mLen, params->EVBME.MESSAGE_indication.mMessage);
	return CA_ERROR_SUCCESS;
}

/**
 * Callback for handling CA8211 PCPS data indications for received 802.15.4 frames
 * @param params  PCPS Data indication struct
 * @param pDeviceRef  Cascoda device reference
 * @return CA_ERROR_SUCCESS
 */
static ca_error handlePcpsDataIndication(struct PCPS_DATA_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (out_mode == OUT_MODE_HEX)
	{
		printTime(NULL);
		ca_print("Rx len %d, CS: %d, ED: %d >", params->PsduLength, params->CS, params->ED);
		for (int i = 0; i < params->PsduLength; i++)
		{
			ca_print(" %02x", params->Psdu[i]);
		}
		ca_print("\n");
	}
	else if (out_mode == OUT_MODE_PCAP)
	{
		pcaprec_hdr_t hdr   = {0, 0, 0, 0};
		ca_error      error = CA_ERROR_SUCCESS;

		fillTimestamp(&hdr, params);
		hdr.incl_len = params->PsduLength + sizeof(ethernet_header);
		hdr.orig_len = params->PsduLength + sizeof(ethernet_header);

		error |= ca_write(&hdr, sizeof(hdr));
		error |= ca_write(&ethernet_header, sizeof(ethernet_header));
		error |= ca_write(params->Psdu, params->PsduLength);
		ca_flush();

		if (error && dPipeName)
		{
			//There has been an issue with writing, pipe probably disconnected at other end.
			//Try to reconnect.
			MLME_RESET_request_sync(1, pDeviceRef); //reset radio so we don't consume all of the memory
			open_pipe(dPipeName);                   //Blocking call to re-open the pipe
			printPcapHeader();                      //Print the pcap header so connection is valid
			initialiseRadio(pDeviceRef);            //Reinitialise radio
		}

		if (debugMode)
		{
			printTime(stderr);
			fprintf(stderr, "Rx len %d, CS: %d, ED: %d >", params->PsduLength, params->CS, params->ED);
			for (int i = 0; i < params->PsduLength; i++)
			{
				fprintf(stderr, " %02x", params->Psdu[i]);
			}
			fprintf(stderr, "\n");
		}
	}

	return CA_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
	struct ca821x_dev *pDeviceRef    = &(sDeviceRef);
	ca_error           error         = CA_ERROR_SUCCESS;
	char              *pipeName      = NULL;
	char              *wiresharkPath = NULL;

	char *serial_num = NULL;

	configure_io();
	snprintf(default_pipe, sizeof(default_pipe), DEFAULT_PIPE "%x", getpid());

	//Argument processing
	for (int i = 1; i < argc; i++)
	{
		int temp = atoi(argv[i]);

		if (strcmp(argv[i], "-p") == 0)
		{
			out_mode = OUT_MODE_PCAP;
		}
		else if (strcmp(argv[i], "-d") == 0)
		{
			debugMode = true;
		}
		else if (strcmp(argv[i], "-n") == 0)
		{
			if (++i >= argc)
			{
				fprintf(stderr, "'-n' option requires filename argument.\n", argv[i]);
				error = CA_ERROR_INVALID_ARGS;
				break;
			}
			pipeName = argv[i];
		}
		else if (strcmp(argv[i], "-w") == 0)
		{
			if (!wiresharkPath)
				wiresharkPath = getDefaultWiresharkPath();
			out_mode = OUT_MODE_PCAP;
			if (!pipeName)
				pipeName = default_pipe;
		}
		else if (strcmp(argv[i], "-W") == 0)
		{
			if (++i >= argc)
			{
				fprintf(stderr, "'-W' option requires program path argument.\n", argv[i]);
				error = CA_ERROR_INVALID_ARGS;
				break;
			}
			wiresharkPath = argv[i];
			out_mode      = OUT_MODE_PCAP;
			if (!pipeName)
				pipeName = default_pipe;
		}
		else if (temp >= 11 && temp <= 26)
		{
			channel = temp;
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			serial_num = argv[++i];
		}
		else
		{
			fprintf(stderr, "Invalid argument \"%s\"\n", argv[i]);
			error = CA_ERROR_INVALID_ARGS;
		}
	} //End argument processing.

	if (error || !channel)
	{
		fprintf(stderr, "Invalid arguments detected.\n");
		displayHelp();
		exit(EXIT_FAILURE);
	}

	if (out_mode == OUT_MODE_PCAP && isatty(fileno(stdout)) && !pipeName)
	{
		fprintf(stderr, "Out mode is pcap, but stdout is tty - redirect to file or pipe!\n");
		displayHelp();
		exit(EXIT_FAILURE);
	}

	if (pipeName)
	{
		create_pipe(pipeName);
	}

	if (wiresharkPath)
	{
		start_wireshark(wiresharkPath);
	}

	if (pipeName)
	{
		open_pipe(pipeName);
	}

	fprintf(stderr, "Initialising ca821x_api.\n");
	while (ca821x_util_init(pDeviceRef, NULL, serial_num))
	{
		sleep(1); //Wait while there isn't a device available to connect
		fprintf(stderr, ".");
	}

	setStartTime();
	if (out_mode == OUT_MODE_PCAP)
	{
		io_raw();
		printPcapHeader();
	}

	//Register callbacks for async messages
	pDeviceRef->callbacks.PCPS_DATA_indication                    = &handlePcpsDataIndication;
	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handleEvbmeMessage;
	ca821x_util_start_upstream_dispatch_worker();

	initialiseRadio(pDeviceRef);

	fprintf(stderr, "\r\nInitialised.\r\n\n");

	while (1)
	{
		sleep(1);
	}
	return 0;
}

#else  // CASCODA_CA_VER != 8210
int main(int argc, char *argv[])
{
	fprintf(stderr, "ERROR: sniffer not compatible with ca8210.");
	return -1;
}
#endif // CASCODA_CA_VER != 8210
