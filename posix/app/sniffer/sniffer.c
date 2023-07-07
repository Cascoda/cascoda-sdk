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

#if _WIN32
#define DEFAULT_PIPE "\\\\.\\pipe\\cascoda_"
#else
#define DEFAULT_PIPE "/tmp/cascoda_"
#endif

/**
 * parameter structures which have been duplicated from mac_messages.h to be able to handle all devices
 */
struct PCPS_DATA_indication_pset_8211
{
	uint8_t CS;                      /**< Carrier sense value of received frame*/
	uint8_t ED;                      /**< Energy detect value of received frame */
	uint8_t PsduLength;              /**< Length of received PSDU */
	uint8_t Psdu[aMaxPHYPacketSize]; /**< Received PSDU */
};

struct PCPS_DATA_indication_pset_8212
{
	uint8_t CS;                      /**< Carrier sense value of received frame*/
	uint8_t ED;                      /**< Energy detect value of received frame */
	uint8_t Timestamp[4];            /**< Timestamp the frame was received at */
	uint8_t PsduLength;              /**< Length of received PSDU */
	uint8_t Psdu[aMaxPHYPacketSize]; /**< Received PSDU */
};

/**
 * device type
 */
typedef enum ca_device_type
{
	DEV_UNKNOWN = 0,
	DEV_CA8210  = 1,
	DEV_CA8211  = 2,
	DEV_CA8212  = 3
} ca_device_type;

ca_device_type DeviceType = DEV_UNKNOWN;

/**
 * Pcap link types, see https://www.tcpdump.org/linktypes.html
 */
#define LINKTYPE_ETHERNET 1               // ethernet used to encapsulate 802.15.4 frames
#define LINKTYPE_IEEE802_15_4_WITHFCS 195 // direct 802.15.4 with FCS
#define LINKTYPE_IEEE802_15_4_TAP 283     // direct 802.15.4 with TAP TLVs

uint32_t link_type = LINKTYPE_IEEE802_15_4_TAP;

/**
 * headers for encapsulating IEEE802.15.4 frames for pcap.
 */
#define MAX_HEADER_LEN 512
#define HEADER_LEN_ETH 14;
#define HEADER_LEN_TAP 28;

#define HEADER_POSITION_RSSI 16
#define HEADER_POSITION_LQI 24

/**
 * headers for encapsulating IEEE802.15.4 frames for pcap.
 * for TAP header doc see:
 * https://github.com/jkcko/ieee802.15.4-tap/blob/master/IEEE%20802.15.4%20TAP%20Link%20Type%20Specification.pdf
 */
uint8_t ethernet_header[] = {0xff, // dst
                             0xff,
                             0xff,
                             0xff,
                             0xff,
                             0xff,
                             0x22, // src
                             0x22,
                             0x22,
                             0x22,
                             0x22,
                             0x22,
                             0x80, // type
                             0x9a};

uint8_t tap_header[] = {0x00, // version
                        0x00, // reserved
                        0x1C, // length
                        0x00,
                        0x00, // FCS TLV
                        0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
                        0x01, // RSSI TLV
                        0x00, 0x04, 0x00,
                        0x00, // TSSI TLV value
                        0x00, 0x00, 0x00,
                        0x0A, // LQI TLV
                        0x00, 0x01, 0x00,
                        0x00, // LQI TLV value
                        0x00, 0x00, 0x00};

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

/** If this is nonzero, then wireshark will be used in ring buffer mode, and a 
 * new capture will be started once every "time_till_new_capture" seconds.
*/
uint32_t time_till_new_capture = 0;

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
 * @param wspath Path to Wireshark
 * @param outpath Path to output directory for Wireshark captures
 */
static void start_wireshark(const char *wspath, const char *outpath);

/**
 * Platform abstraction function to register the start time of the program.
 */
static void setStartTime(void);

/**
 * Platform abstraction function to fill in the timing information for the pcap header from Operating System.
 * @param pcapHeader A pointer to the pcap header to be filled.
 * @param params A pointer to the received PCPS indication
 */
static void fillTimestampFromOS(pcaprec_hdr_t *pcapHeader);

/**
 * Platform abstraction function to fill in the timing information for the pcap header from MAC
 * @param pcapHeader A pointer to the pcap header to be filled.
 * @param params A pointer to the received PCPS indication
 */
static void fillTimestampFromMAC(pcaprec_hdr_t *pcapHeader, struct PCPS_DATA_indication_pset_8212 *params);

/**
 * Platform abstraction function to configure the default system output.
 */
static void configure_io(void);

/**
 * Platform abstraction function to disable all output processing.
 */
static void io_raw(void);

/**
 * fills in RSSI/ED value for LINKTYPE_IEEE802_15_4_TAP.
 * @param ed ED/RSSI value from device (0-255)
 */
static void fillRSSITap(uint8_t ed);

/**
 * fills in LIQ/CS value for LINKTYPE_IEEE802_15_4_TAP.
 * @param cs CS/LQI value from device (0-255)
 */
static void fillLQITap(uint8_t cs);

/**
 * reads the device version (note that CASCODA_CA_VER should not be used outside baremetal)
 */
static void GetDeviceVersion(struct ca821x_dev *pDeviceRef);

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
	fprintf(stderr, "\t-b [DURATION]    Ring-buffer mode. Wireshark will save and start a new capture\n");
	fprintf(stderr, "\t                 every number of seconds specified by the DURATION argument.\n");
	fprintf(stderr, "\t                 Note: This HAS to be used in conjunction with -o, to specify\n");
	fprintf(stderr, "\t                 the output directory path where the pcap files will be created.\n\n");
	fprintf(stderr, "\t-d               Debug mode, print verbose information to stderr.\n\n");
	fprintf(stderr, "\t-e               Use PCap datalink type ETHERNET (default is IEEE802_15_4_TAP).\n\n");
	fprintf(stderr,
	        "\t-i               Use PCap datalink type IEEE802_15_4_WITHFCS (default is IEEE802_15_4_TAP).\n\n");
	fprintf(stderr, "\t-o [PATH]        If provided, wireshark will save the current capture in the output\n");
	fprintf(stderr, "\t                 directory provided. Otherwise, Wireshark will not save the capture.\n");
	fprintf(stderr, "\t                 NOTE: This option is mandatory if -b is used.\n\n");
	fprintf(stderr, "\t-n [PIPENAME]    Output to a named pipe/fifo, which can be read by\n");
	fprintf(stderr, "\t                 wireshark or another program. Most useful in conjunction\n");
	fprintf(stderr, "\t                 with '-p'.\n\n");
	fprintf(stderr, "\t-p               PCap mode, output pcap data instead of descriptive hex\n");
	fprintf(stderr, "\t                 dump. If this is used in conjunction with pipes, can be\n");
	fprintf(stderr, "\t                 used to stream to wireshark\n\n");
	fprintf(stderr, "\t-s [SERIALNO]    This application will use the sniffer with the serial number specified.\n");
	fprintf(stderr, "\t                 If not specified, the first sniffer detected will be used.\n\n");
	fprintf(stderr, "\t-w               Open WireShark to process the packet capture. Implies -p\n");
	fprintf(stderr, "\t                 and -n (random name for pipe if not provided separately).\n\n");
	fprintf(stderr, "\t-W [PATH]        Open WireShark at the path to process the packet capture.\n");
	fprintf(stderr, "\t                 Implies -w.\n\n");
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

static void start_wireshark(const char *wspath, const char *outpath)
{
	int                 rval;
	int                 arglen = strlen(dPipeName) + 200;
	char                args[arglen];
	char                savePath[MAX_PATH];
	char                saveSuffix[MAX_PATH];
	STARTUPINFO         StartupInfo;
	PROCESS_INFORMATION ProcessInfo;

	if (outpath)
	{
		strcpy(savePath, outpath);
		snprintf(saveSuffix, MAX_PATH, "\\ch_%d.pcapng", channel);
		strcat(savePath, saveSuffix);
	}

	memset(&StartupInfo, 0, sizeof(StartupInfo));
	memset(&ProcessInfo, 0, sizeof(ProcessInfo));

	// Wireshark commandline arguments, see https://www.wireshark.org/docs/wsug_html_chunked/ChCustCommandLine.html
	// -i selects the interface (which is the pipe that was created by the sniffer)
	// -w selects an output file
	// -k makes it start capturing immediately
	// -b duration:<value>, causes wireshark to run in "ring buffer" mode, and save/write
	// to a new output file every <value> seconds.
	if (time_till_new_capture != 0)
		snprintf(args, arglen, " -i %s -w %s -k -b duration:%d", dPipeName, savePath, time_till_new_capture);
	else if (outpath)
		snprintf(args, arglen, " -i %s -w %s -k", dPipeName, savePath);
	else
		snprintf(args, arglen, " -i %s -k", dPipeName);

	fprintf(stderr, "Starting wireshark...\n");
	rval = CreateProcess(wspath, args, NULL, NULL, false, 0, NULL, NULL, &StartupInfo, &ProcessInfo);

	if (!rval)
	{
		fprintf(stderr, "Failed to start Wireshark, check path.\n");
		DWORD dw = GetLastError();
		printf("%d\n", dw);
	}
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

static void fillTimestampFromOS(pcaprec_hdr_t *pcapHeader)
{
	LARGE_INTEGER now;

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

static void configure_io(void)
{
	output = GetStdHandle(STD_OUTPUT_HANDLE);
}

static void io_raw(void)
{
	setmode(STDOUT_FILENO, O_BINARY);
}

//End of windows abstraction
#else  //posix abstraction

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

static void start_wireshark(const char *wspath, const char *outpath)
{
	int pid = fork();
	if (pid == 0)
	{
		int  rval;
		char savePath[200];
		char saveSuffix[200];
		// We are in the child process, execute the command
		int  cmdlen = strlen(wspath) + strlen(dPipeName) + 200;
		char buffer[cmdlen];

		if (outpath)
		{
			strcpy(savePath, outpath);
			snprintf(saveSuffix, 200, "\\ch_%d.pcapng", channel);
			strcat(savePath, saveSuffix);
		}

		// Wireshark commandline arguments, see https://www.wireshark.org/docs/wsug_html_chunked/ChCustCommandLine.html
		// -i selects the interface (which is the pipe that was created by the sniffer)
		// -w selects an output file
		// -k makes it start capturing immediately
		// -b duration:<value>, causes wireshark to run in "ring buffer" mode, and save/write
		// to a new output file every <value> seconds.
		if (time_till_new_capture != 0)
			snprintf(
			    buffer, cmdlen, "%s -i %s -w %s -k -b duration:%d", wspath, dPipeName, savePath, time_till_new_capture);
		else if (outpath)
			snprintf(buffer, cmdlen, "%s -i %s -w %s -k", wspath, dPipeName, savePath);
		else
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

static void fillTimestampFromOS(pcaprec_hdr_t *pcapHeader)
{
	struct timespec now;

	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
	{
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
	now                 = timeSub(&now, &start);
	pcapHeader->ts_sec  = (uint32_t)(now.tv_sec);
	pcapHeader->ts_usec = (uint32_t)(now.tv_nsec / 1000);
}

static void configure_io(void)
{
	output = stdout;
}

static void io_raw(void)
{
}
#endif //End of posix abstraction

static void fillTimestampFromMAC(pcaprec_hdr_t *pcapHeader, struct PCPS_DATA_indication_pset_8212 *params)
{
	const uint64_t micros = 1000000;
	uint32_t       ts     = GETLE32(params->Timestamp); // timestamp in symbols
	uint64_t       ts_us  = (uint64_t)ts * aSymbolPeriod_us;
	uint64_t       ts_s   = ts_us / micros;

	pcapHeader->ts_sec  = (uint32_t)ts_s;
	pcapHeader->ts_usec = (uint32_t)(ts_us - (ts_s * micros));
}

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
	    link_type,  //Network
	};

	ca_write(&hdr, sizeof(hdr));
	ca_flush();
}

/**
 * Reset and initialise the radio for sniffing.
 * @param from_startup initialise from startup when 1, re-initialisation only when 0.
 * @param pDeviceRef cascoda device reference.
 */
static void initialiseRadio(uint8_t from_startup, struct ca821x_dev *pDeviceRef)
{
	if (from_startup)
	{
		ca_error status;
		status = EVBME_HOST_CONNECTED_notify(pDeviceRef); // reset device with hardware reset
		if (status)
		{
			fprintf(stderr, "Failed to connect, status %02X\n", status);
			exit(EXIT_FAILURE);
		}
		GetDeviceVersion(pDeviceRef);
	}
	MLME_RESET_request_sync(1, pDeviceRef);

	if (DeviceType == DEV_CA8210) /* TDME interface */
	{
		//Reset test mode
		TDME_TESTMODE_request_sync(TDME_TEST_OFF, pDeviceRef);
		// Set current channel to selected
		TDME_SET_request_sync(TDME_CHANNEL, 1, &channel, pDeviceRef);
		//Enable receiving FCS field
		uint8_t macCfg = 0;
		TDME_GETSFR_request_sync(0, 0xD9, &macCfg, pDeviceRef);
		macCfg |= 0x10;
		TDME_SETSFR_request_sync(0, 0xD9, macCfg, pDeviceRef);
		//Set test mode to Rx
		TDME_TESTMODE_request_sync(TDME_TEST_RX, pDeviceRef);
	}
	else /* PCPS interface */
	{
		uint8_t att = 1;
		//Set RxMode to PCPS
		HWME_SET_request_sync(HWME_RXMODE, 1, &att, pDeviceRef);
		//Set current channel to selected
		MLME_SET_request_sync(phyCurrentChannel, 0, 1, &channel, pDeviceRef);
		//Enable promiscuous mode, to receive all frames
		att = 1;
		MLME_SET_request_sync(macPromiscuousMode, 0, 1, &att, pDeviceRef);
		//Enable receiving FCS field
		uint8_t macCfg = 0;
		TDME_GETSFR_request_sync(0, 0xD9, &macCfg, pDeviceRef);
		macCfg |= 0x10;
		TDME_SETSFR_request_sync(0, 0xD9, macCfg, pDeviceRef);
		//Set Rx enabled
		if (DeviceType == DEV_CA8212)
		{
			//Set LQI limit to 0
			att = 0;
			HWME_SET_request_sync(HWME_LQILIMIT, 1, &att, pDeviceRef);
		}
		att = 1;
		MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, &att, pDeviceRef);
	}
}

/**
 * Output packet in Hex Mode
 * @param payload pointer to payload
 * @param length payload length
 * @param cs CS/LQI value
 * @param ed ED/RSSI value
 */
static void OutputHex(uint8_t *payload, uint8_t length, uint8_t cs, uint8_t ed)
{
	printTime(NULL);
	ca_print("Rx len %d, CS: %d, ED: %d >", length, cs, ed);
	for (int i = 0; i < length; i++)
	{
		ca_print(" %02x", payload[i]);
	}
	ca_print("\n");
}

/**
 * Output packet in Debug Mode
 * @param payload pointer to payload
 * @param length payload length
 * @param cs CS/LQI value
 * @param ed ED/RSSI value
 */
static void OutputDebug(uint8_t *payload, uint8_t length, uint8_t cs, uint8_t ed)
{
	printTime(stderr);
	fprintf(stderr, "Rx len %d, CS: %d, ED: %d >", length, cs, ed);
	for (int i = 0; i < length; i++)
	{
		fprintf(stderr, " %02x", payload[i]);
	}
	fprintf(stderr, "\n");
}

/**
 * Output packet in PCap Mode
 * @param payload pointer to the pcap header
 * @param payload pointer to payload
 * @param length payload length
 * @param cs CS/LQI value
 * @param ed ED/RSSI value
 * @param pDeviceRef cascoda device reference.
 */
static void OutputPcap(pcaprec_hdr_t     *hdr,
                       uint8_t           *payload,
                       uint8_t            length,
                       uint8_t            cs,
                       uint8_t            ed,
                       struct ca821x_dev *pDeviceRef)
{
	ca_error error = CA_ERROR_SUCCESS;
	uint8_t  pkt_header[MAX_HEADER_LEN];
	uint32_t header_len = 0;

	if (link_type == LINKTYPE_ETHERNET)
	{
		header_len = HEADER_LEN_ETH;
		memcpy(pkt_header, ethernet_header, header_len);
	}
	else if (link_type == LINKTYPE_IEEE802_15_4_TAP)
	{
		fillRSSITap(ed);
		fillLQITap(cs);
		header_len = HEADER_LEN_TAP;
		memcpy(pkt_header, tap_header, header_len);
	}
	else
	{
		header_len = 0;
	}

	hdr->incl_len = length + header_len;
	hdr->orig_len = length + header_len;

	error |= ca_write(hdr, sizeof(pcaprec_hdr_t));
	error |= ca_write(&pkt_header, header_len);
	error |= ca_write(payload, length);
	ca_flush();

	if (error && dPipeName)
	{
		//There has been an issue with writing, pipe probably disconnected at other end.
		//Try to reconnect.
		open_pipe(dPipeName);           //Blocking call to re-open the pipe
		printPcapHeader();              //Print the pcap header so connection is valid
		initialiseRadio(0, pDeviceRef); //Reinitialise radio
	}
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
static ca_error handlePcpsDataIndication8211(struct PCPS_DATA_indication_pset_8211 *params,
                                             struct ca821x_dev                     *pDeviceRef)
{
	if (out_mode == OUT_MODE_HEX)
	{
		OutputHex(params->Psdu, params->PsduLength, params->CS, params->ED);
	}
	else if (out_mode == OUT_MODE_PCAP)
	{
		pcaprec_hdr_t hdr = {0, 0, 0, 0};
		fillTimestampFromOS(&hdr);
		OutputPcap(&hdr, params->Psdu, params->PsduLength, params->CS, params->ED, pDeviceRef);
		if (debugMode)
		{
			OutputDebug(params->Psdu, params->PsduLength, params->CS, params->ED);
		}
	}
	return CA_ERROR_SUCCESS;
}

/**
 * Callback for handling CA8212 PCPS data indications for received 802.15.4 frames
 * @param params  PCPS Data indication struct
 * @param pDeviceRef  Cascoda device reference
 * @return CA_ERROR_SUCCESS
 */
static ca_error handlePcpsDataIndication8212(struct PCPS_DATA_indication_pset_8212 *params,
                                             struct ca821x_dev                     *pDeviceRef)
{
	if (out_mode == OUT_MODE_HEX)
	{
		OutputHex(params->Psdu, params->PsduLength, params->CS, params->ED);
	}
	else if (out_mode == OUT_MODE_PCAP)
	{
		pcaprec_hdr_t hdr = {0, 0, 0, 0};
		fillTimestampFromMAC(&hdr, params);
		OutputPcap(&hdr, params->Psdu, params->PsduLength, params->CS, params->ED, pDeviceRef);
		if (debugMode)
		{
			OutputDebug(params->Psdu, params->PsduLength, params->CS, params->ED);
		}
	}
	return CA_ERROR_SUCCESS;
}

/**
 * Callback for handling PCPS data indications, switching between CA8211 and CA8212 for further processing
 * @param msg generic MAC_Message struct, as different parameter sets have to be extracted
 * @param pDeviceRef  Cascoda device reference
 * @return CA_ERROR_SUCCESS
 */
static ca_error handleAndSwitchPcpsDataIndication(const struct MAC_Message *msg, struct ca821x_dev *pDeviceRef)
{
	if (msg->CommandId == SPI_PCPS_DATA_INDICATION)
	{
		if (DeviceType == DEV_CA8212)
			handlePcpsDataIndication8212((struct PCPS_DATA_indication_pset_8212 *)(msg->PData.Payload), pDeviceRef);
		else
			handlePcpsDataIndication8211((struct PCPS_DATA_indication_pset_8211 *)(msg->PData.Payload), pDeviceRef);
	}

	return CA_ERROR_SUCCESS;
}

/**
 * Callback for handling CA8210 TDME rx packet indications for received 802.15.4 frames
 * @param params TDME RXPKT indication struct
 * @param pDeviceRef  Cascoda device reference
 * @return CA_ERROR_SUCCESS
 */
static ca_error handleTdmeRxpktIndication(struct TDME_RXPKT_indication_pset *params, struct ca821x_dev *pDeviceRef)
{
	if (!params->Status) /* handle only packets without any errors */
	{
		if (out_mode == OUT_MODE_HEX)
		{
			OutputHex(
			    params->TestPacketData, params->TestPacketLength, params->TestPacketCSValue, params->TestPacketEDValue);
		}
		else if (out_mode == OUT_MODE_PCAP)
		{
			pcaprec_hdr_t hdr = {0, 0, 0, 0};
			fillTimestampFromOS(&hdr);
			OutputPcap(&hdr,
			           params->TestPacketData,
			           params->TestPacketLength,
			           params->TestPacketCSValue,
			           params->TestPacketEDValue,
			           pDeviceRef);
			if (debugMode)
			{
				OutputDebug(params->TestPacketData,
				            params->TestPacketLength,
				            params->TestPacketCSValue,
				            params->TestPacketEDValue);
			}
		}
	} // !params->Status
	return CA_ERROR_SUCCESS;
}

static void fillRSSITap(uint8_t ed)
{
	float valdbm;
	union
	{
		float   vfloat;
		uint8_t vbytes[4];
	} convert_t;

	/* calculate dbm value from ed value */
	valdbm = ((float)ed - 256.0) / 2.0;
	/* put float (32 bit) value into union */
	convert_t.vfloat = valdbm;
	/* copy uint8_t array part of union into tap header */
	memcpy((tap_header + HEADER_POSITION_RSSI), convert_t.vbytes, 4);
}

static void fillLQITap(uint8_t cs)
{
	/* staight forward mapping as value is 8-bit 0-255 */
	tap_header[HEADER_POSITION_LQI] = cs;
}

static void GetDeviceVersion(struct ca821x_dev *pDeviceRef)
{
	struct hwme_chipid
	{
		uint8_t hw_version; // hardware version / pid
		uint8_t fw_version; // firmware version / version number
	};
	struct hwme_chipid chipid;
	uint8_t            attlen;
	uint8_t            status;

	status = HWME_GET_request_sync(HWME_CHIPID, &attlen, (uint8_t *)&chipid, pDeviceRef);
	if (status)
	{
		fprintf(stderr, "GetDeviceVersion Status: %02X\n", status);
		exit(EXIT_FAILURE);
	}
	if ((chipid.hw_version == 1) && (chipid.fw_version <= 2))
		DeviceType = DEV_CA8210;
	else if ((chipid.hw_version == 1) && (chipid.fw_version == 3))
		DeviceType = DEV_CA8211;
	else if (chipid.hw_version == 2)
		DeviceType = DEV_CA8212;
	else
		DeviceType = DEV_UNKNOWN;

	if (DeviceType == DEV_UNKNOWN)
	{
		fprintf(stderr, "unknown sniffing device (%u.%u)\n", chipid.hw_version, chipid.fw_version);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	struct ca821x_dev *pDeviceRef    = &(sDeviceRef);
	ca_error           error         = CA_ERROR_SUCCESS;
	char              *pipeName      = NULL;
	char              *wiresharkPath = NULL;
	char              *outputDirPath = NULL;

	union ca821x_util_init_extra_arg sniffer_arg = {.generic = NULL};

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
				fprintf(stderr, "'-n' option requires filename argument.\n");
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
				fprintf(stderr, "'-W' option requires program path argument.\n");
				error = CA_ERROR_INVALID_ARGS;
				break;
			}
			wiresharkPath = argv[i];
			out_mode      = OUT_MODE_PCAP;
			if (!pipeName)
				pipeName = default_pipe;
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			sniffer_arg.generic = argv[++i];
		}
		else if (strcmp(argv[i], "-e") == 0)
		{
			link_type = LINKTYPE_ETHERNET;
		}
		else if (strcmp(argv[i], "-i") == 0)
		{
			link_type = LINKTYPE_IEEE802_15_4_WITHFCS;
		}
		else if (strcmp(argv[i], "-b") == 0)
		{
			if (++i >= argc)
			{
				fprintf(stderr, "'-b' option requires time (s) argument.\n");
				error = CA_ERROR_INVALID_ARGS;
				break;
			}
			time_till_new_capture = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			if (++i >= argc)
			{
				printf("checkpoint\n");
				fprintf(stderr, "'-o' option requires output directory path argument.\n");
				error = CA_ERROR_INVALID_ARGS;
				break;
			}
			outputDirPath = argv[i];
		}
		else if (temp >= 11 && temp <= 26)
		{
			channel = temp;
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

	if (time_till_new_capture != 0 && outputDirPath == NULL)
	{
		fprintf(stderr, "You have to provide an output directory path when using ring-buffer mode.\n");
		displayHelp();
		exit(EXIT_FAILURE);
	}

	if (pipeName)
	{
		create_pipe(pipeName);
	}

	if (wiresharkPath)
	{
		start_wireshark(wiresharkPath, outputDirPath);
	}

	if (pipeName)
	{
		open_pipe(pipeName);
	}

	fprintf(stderr, "Initialising ca821x_api.\n");
	while (ca821x_util_init(pDeviceRef, NULL, sniffer_arg))
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

	initialiseRadio(1, pDeviceRef);

	//Register callbacks for async messages
	if (DeviceType == DEV_CA8210)
	{
		pDeviceRef->callbacks.TDME_RXPKT_indication = &handleTdmeRxpktIndication;
	}
	else
	{
		/* can't use pDeviceRef->callbacks.PCPS_DATA_indication as params differ between 8211 and 8212 */
		/* so have to use generic callback to get MAC_Message and then dissect */
		pDeviceRef->callbacks.generic_dispatch = &handleAndSwitchPcpsDataIndication;
	}

	EVBME_GetCallbackStruct(pDeviceRef)->EVBME_MESSAGE_indication = &handleEvbmeMessage;
	ca821x_util_start_upstream_dispatch_worker();

	fprintf(stderr, "\r\nInitialised.\r\n\n");

	while (1)
	{
		sleep(1);
	}
	return 0;
}
