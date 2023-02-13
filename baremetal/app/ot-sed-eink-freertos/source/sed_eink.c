/******************************************************************************/
/******************************************************************************/
/****** Cascoda Ltd. 2019                                                ******/
/******************************************************************************/
/******************************************************************************/
/****** Openthread standalone SED w/ basic CoAP server discovery & reporting **/
/******************************************************************************/
/******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_evbme.h"
#include "cascoda-bm/cascoda_interface.h"
#include "cascoda-bm/cascoda_os.h"
#include "cascoda-bm/cascoda_sensorif.h"
#include "cascoda-bm/cascoda_serial.h"
#include "cascoda-bm/cascoda_types.h"
#include "cascoda-bm/test15_4_evbme.h"
#include "cascoda-util/cascoda_rand.h"
#include "cascoda-util/cascoda_time.h"
#include "ca821x_api.h"
#include "ca821x_endian.h"

#include "openthread/coap.h"
#include "openthread/instance.h"
#include "openthread/link.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"
#include "platform.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "cbor.h"
#include "sif_il3820.h"
#include "sif_il3820_image.h"
#include "uzlib.h"

#define SuccessOrExit(aCondition) \
	do                            \
	{                             \
		if ((aCondition) != 0)    \
		{                         \
			goto exit;            \
		}                         \
	} while (0)

#define TCQR_BUFFER_SIZE 50

/******************************************************************************/
/****** Power Consumption Configuration                                  ******/
/******************************************************************************/
// How long to wait between discover request retries
const int DISCOVER_FAIL_RETRY_MS = 3000;
// How long to put the device to sleep if it cannot establish a connection at all
// Not currently implemented
const int DISCOVER_TIMEOUT_SLEEP_MS = 30 * 1000;

// How long to sleep for after having received an image
const int IMAGE_OK_SLEEP_MS = 8 * 1000;
// Devices sleep for IMAGE_OK_SLEEP_MS + [0, IMAGE_RANDOM_SLEEP_MS)
const int IMAGE_RANDOM_SLEEP_MS = 4 * 1000;
// How long to wait between resending image GET requests
const int IMAGE_FAIL_RETRY_MS = 10 * 1000;

const char *uriCascodaDiscover    = "ca/di";
const char *uriCascodaTemperature = "ca/te";
const char *uriCascodaImage       = "ca/img";
char        uriCascodaImageRequestQuery[10];

/******************************************************************************/
/****** Single instance                                                  ******/
/******************************************************************************/
otInstance       *OT_INSTANCE;
struct ca821x_dev sDeviceRef;

static bool         isConnected  = false;
static int          timeoutCount = 0;
static otIp6Address serverIp;
static int          deviceID;

/******************************************************************************/
/****** FreeRTOS-related globals                                         ******/
/******************************************************************************/
TaskHandle_t      CommsTaskHandle;
SemaphoreHandle_t CommsMutexHandle;

// The size matches the maximum size of a CoAP packet
static unsigned char message_buffer[1024];
// length of the message currently in the buffer
static uint16_t message_length = 0;
// The resolution of the image is 296x128
#define IMAGE_SIZE (296 * 128 / 8)
static unsigned char image_buffer[IMAGE_SIZE];

void initialise_communications();

int ot_reinitialise(struct ca821x_dev *pDeviceRef)
{
	otLinkSyncExternalMac(OT_INSTANCE);
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Initialisation function
 *******************************************************************************
 ******************************************************************************/
static void App_Initialise(u8_t status, struct ca821x_dev *pDeviceRef)
{
	struct ModuleSpecialPins special_pins = BSP_GetModuleSpecialPins();
	/* register LED_G */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_GREEN, MODULE_PIN_TYPE_LED);
	/* register LED_R */
	BSP_ModuleRegisterGPIOOutput(special_pins.LED_RED, MODULE_PIN_TYPE_LED);

	if (status == CA_ERROR_FAIL)
	{
		BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_ON);
		return;
	}

	BSP_ModuleSetGPIOPin(special_pins.LED_RED, LED_OFF);
	BSP_ModuleSetGPIOPin(special_pins.LED_GREEN, LED_ON);

	EVBME_SwitchClock(pDeviceRef, 1);
}

// Callback called by OpenThread when there is work to
void otTaskletsSignalPending(otInstance *aInstance)
{
	(void)aInstance;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Handle the response to the server discover, and register the server
 * locally.
 *******************************************************************************
 ******************************************************************************/
static void handleServerDiscoverResponse(void                *aContext,
                                         otMessage           *aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         otError              aError)
{
	//Cbor Variables
	CborError  err;
	CborParser parser;
	CborValue  value;
	CborValue  ip_result;
	CborValue  id_result;

	uint16_t      length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	uint16_t      offset = otMessageGetOffset(aMessage);
	unsigned char buffer[40];

	if (aError != OT_ERROR_NONE)
		return;
	if (isConnected)
		return;
	if (length < sizeof(otIp6Address))
		return;

	//Store the received serialized CBOR message into it
	otMessageRead(aMessage, offset, buffer, length);
	//Initialise the CBOR parser
	SuccessOrExit(cbor_parser_init(buffer, length, 0, &parser, &value));

	//Extract the values corresponding to the keys from the CBOR map
	SuccessOrExit(cbor_value_map_find_value(&value, "ip", &ip_result));
	SuccessOrExit(cbor_value_map_find_value(&value, "devID", &id_result));
	//Retrieve IP and assigned ID

	if (cbor_value_get_type(&ip_result) != CborInvalidType)
	{
		size_t len = sizeof(otIp6Address);
		SuccessOrExit(cbor_value_copy_byte_string(&ip_result, serverIp.mFields.m8, &len, NULL));
	}
	if (cbor_value_get_type(&id_result) != CborInvalidType)
	{
		SuccessOrExit(cbor_value_get_int(&id_result, &deviceID));
		snprintf(uriCascodaImageRequestQuery, sizeof(uriCascodaImageRequestQuery), "id=%d", deviceID);
	}

	isConnected  = true;
	timeoutCount = 0;

exit:
	return;
}

/******************************************************************************/
/***************************************************************************/ /**
 * \brief Send a multicast cascoda 'server discover' coap message. This is a
 * non-confirmable get request, and the get responses will be handled by the
 * 'handleServerDiscoverResponse' function.
 *******************************************************************************
 ******************************************************************************/
static otError sendServerDiscover(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
	otMessageInfo messageInfo;
	otIp6Address  coapDestinationIp;
	otExtAddress  eui64;
	uint8_t       buffer[64];
	CborError     err;
	CborEncoder   encoder, mapEncoder;

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	otLinkGetFactoryAssignedIeeeEui64(OT_INSTANCE, &eui64);

	//Build CoAP header
	//Realm local all-nodes multicast - this of course generates some traffic, so shouldn't be overused
	SuccessOrExit(error = otIp6AddressFromString("FF03::1", &coapDestinationIp));
	otCoapMessageInit(message, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_GET);
	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaDiscover));
	otCoapMessageAppendContentFormatOption(message, OT_COAP_OPTION_CONTENT_FORMAT_CBOR);
	otCoapMessageSetPayloadMarker(message);

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = coapDestinationIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	//CBOR initialisation
	cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);
	SuccessOrExit(err = cbor_encoder_create_map(&encoder, &mapEncoder, 1));
	SuccessOrExit(err = cbor_encode_text_stringz(&mapEncoder, "eui"));
	SuccessOrExit(err = cbor_encode_byte_string(&mapEncoder, eui64.m8, sizeof(otExtAddress)));
	SuccessOrExit(err = cbor_encoder_close_container(&encoder, &mapEncoder));
	size_t length = cbor_encoder_get_buffer_size(&encoder, buffer);
	SuccessOrExit(error = otMessageAppend(message, buffer, length));

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleServerDiscoverResponse, NULL);

exit:
	if ((err || error) && message)
	{
		//error, we have to free
		otMessageFree(message);
	}

	if (err && !error)
		error = OT_ERROR_FAILED;

	return error;
}

void decompress_data()
{
	// Decompress the message using uzlib

	// Decompressed length, can be mismatched by one byte
	unsigned int dlen = IMAGE_SIZE + 1;
	// How many bytes to decompress in one pass
	const int OUT_CHUNK_SIZE = 1;
	int       res;

	/* -- decompress data -- */

	struct uzlib_uncomp d;
	uzlib_uncompress_init(&d, NULL, 0);

	/* all 3 fields below must be initialized by user */
	d.source         = message_buffer;
	d.source_limit   = message_buffer + IMAGE_SIZE - 4;
	d.source_read_cb = NULL;

	res = uzlib_gzip_parse_header(&d);
	if (res != TINF_OK)
	{
		// Could not parse GZIP header
		configASSERT(0);
	}

	d.dest_start = d.dest = image_buffer;

	while (dlen)
	{
		unsigned int chunk_len = dlen < OUT_CHUNK_SIZE ? dlen : OUT_CHUNK_SIZE;
		d.dest_limit           = d.dest + chunk_len;
		res                    = uzlib_uncompress_chksum(&d);
		dlen -= chunk_len;
		if (res != TINF_OK)
		{
			break;
		}
	}

	if (res != TINF_DONE)
	{
		// Error during decompression
		configASSERT(0);
	}
}

static void handleImageResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
	//CBOR variables
	CborError  err;
	CborParser parser;
	CborValue  value;
	CborValue  result;
	CborValue  id_result;

	if (aError == OT_ERROR_RESPONSE_TIMEOUT && timeoutCount++ > 3)
		isConnected = false;
	else if (aError == OT_ERROR_NONE)
		timeoutCount = 0;

	if (aError != OT_ERROR_NONE)
		return;

	if (otCoapMessageGetCode(aMessage) != OT_COAP_CODE_CONTENT)
		return;

	isConnected = true;

	// Put the data in the message buffer
	message_length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	otMessageRead(aMessage, otMessageGetOffset(aMessage), &message_buffer, message_length);

	//Initialise the CBOR parser
	SuccessOrExit(err = cbor_parser_init(message_buffer, message_length, 0, &parser, &value));

	//Extract the values corresponding to the keys from the CBOR map
	SuccessOrExit(err = cbor_value_map_find_value(&value, "img", &result));
	SuccessOrExit(err = cbor_value_map_find_value(&value, "devID", &id_result));

	if (cbor_value_get_type(&result) != CborInvalidType)
	{
		size_t len = sizeof(message_buffer);
		SuccessOrExit(err = cbor_value_copy_byte_string(&result, message_buffer, &len, NULL));
	}

	if (cbor_value_get_type(&id_result) != CborInvalidType)
	{
		SuccessOrExit(err = cbor_value_get_int(&id_result, &deviceID));
		snprintf(uriCascodaImageRequestQuery, sizeof(uriCascodaImageRequestQuery), "id=%d", deviceID);
	}

	// Turn off the radio if you have successfully received an image
	otInstanceFinalize(OT_INSTANCE);

	// Decompress the data and put it in the image buffer
	decompress_data();

	// Write the received data to the display
	SIF_IL3820_Initialise(&lut_full_update);
	SIF_IL3820_Display(image_buffer);
	SIF_IL3820_DeepSleep();

	// Get a random number, to randomise the sleep time
	uint16_t random;
	RAND_GetBytes(sizeof(random), &random);

	// Sleep until you must get a new image
	int random_delay_ms = random % IMAGE_RANDOM_SLEEP_MS;
	EVBME_PowerDown(PDM_DPD, IMAGE_OK_SLEEP_MS + random_delay_ms, &sDeviceRef);

exit:
	if (err)
		return;
	// Should not get here
	for (;;)
		;
}

static otError sendImageRequest(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage    *message = NULL;
	otMessageInfo messageInfo;

	//allocate message buffer
	message = otCoapNewMessage(OT_INSTANCE, NULL);
	if (message == NULL)
	{
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	//Build CoAP header
	otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
	otCoapMessageGenerateToken(message, 2);
	SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, uriCascodaImage));

	//Append URI option that identifies the device's ID
	SuccessOrExit(error = otCoapMessageAppendUriQueryOption(message, uriCascodaImageRequestQuery));

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = serverIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

	//send
	error = otCoapSendRequest(OT_INSTANCE, message, &messageInfo, &handleImageResponse, NULL);

exit:
	if (error && message)
	{
		//error, we have to free
		otMessageFree(message);
	}
	return error;
}

void ImageHandlerTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	for (;;)
	{
		if (isConnected)
		{
			xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);
			sendImageRequest();
			xSemaphoreGive(CommsMutexHandle);

			// Block after requesting data
			const TickType_t delay = IMAGE_FAIL_RETRY_MS / portTICK_PERIOD_MS;
			vTaskDelay(delay);
		}
		else
		{
			xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);
			sendServerDiscover();
			xSemaphoreGive(CommsMutexHandle);

			// Block after attempting to connect
			const TickType_t delay = DISCOVER_FAIL_RETRY_MS / portTICK_PERIOD_MS;
			vTaskDelay(delay);
		}
	}
}

static void CommsTask(void *unused)
{
	(void)unused;

	/* This task calls secure-side functions (namely, functions in the BSP)
	 * therefore it must allocate a secure context before doing so. */
	portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);

	initialise_communications();

	xTaskCreate(ImageHandlerTask, "Image", 4 * 1024, NULL, 3, NULL);

	for (;;)
	{
		xSemaphoreTake(CommsMutexHandle, portMAX_DELAY);

		cascoda_io_handler(&sDeviceRef);
		otTaskletsProcess(OT_INSTANCE);

		xSemaphoreGive(CommsMutexHandle);
	}
} // End of CommsTask()

void initialise_communications()
{
	u8_t             StartupStatus;
	otLinkModeConfig linkMode               = {0};
	char             tcQR[TCQR_BUFFER_SIZE] = {};

	ca821x_api_init(&sDeviceRef);
	cascoda_reinitialise    = ot_reinitialise;

	// Initialisation of Chip and EVBME
	StartupStatus = EVBMEInitialise(CA_TARGET_NAME, &sDeviceRef);

	// Insert Application-Specific Initialisation Routines here
	App_Initialise(StartupStatus, &sDeviceRef);
	PlatformRadioInitWithDev(&sDeviceRef);
	OT_INSTANCE = otInstanceInitSingle();

	SENSORIF_SPI_Config(1);

	if (!otDatasetIsCommissioned(OT_INSTANCE))
	{
		// If not commissioned, print the commissioning QR code to the display
		SIF_IL3820_Initialise(&lut_full_update);
		PlatformGetQRString(tcQR, TCQR_BUFFER_SIZE, OT_INSTANCE);
		SIF_IL3820_overlay_qr_code(tcQR, cascoda_img_2in9, 90, 20);
		SIF_IL3820_ClearAndDisplayImage(cascoda_img_2in9);
	}

	// Print the joiner credentials, delaying for up to 5 seconds
	PlatformPrintJoinerCredentials(&sDeviceRef, OT_INSTANCE, 5000);
	// Try to join network
	do
	{
		otError otErr = PlatformTryJoin(&sDeviceRef, OT_INSTANCE);
		if (otErr == OT_ERROR_NONE || otErr == OT_ERROR_ALREADY)
			break;

		PlatformSleep(30000);
	} while (1);

	linkMode.mRxOnWhenIdle = true;
	otLinkSetPollPeriod(OT_INSTANCE, 5000);
	otThreadSetChildTimeout(OT_INSTANCE, 5);
	otThreadSetLinkMode(OT_INSTANCE, linkMode);

	otThreadSetEnabled(OT_INSTANCE, true);
	otCoapStart(OT_INSTANCE, OT_DEFAULT_COAP_PORT);
	otTaskletsProcess(OT_INSTANCE);
}

/******************************************************************************/
/******************************************************************************/
/****** main()                                                           ******/
/******************************************************************************/
/****** Brief:  Main Program Endless Loop                                ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: Does not return                                          ******/
/******************************************************************************/
/******************************************************************************/
int main(void)
{
	// Create the mutex that controls access to the OpenThread API
	CommsMutexHandle = xSemaphoreCreateMutex();

	// Create the communications task. It controls the radio and the
	// Thread network stack.
	xTaskCreate(CommsTask, "Comms", 1024, NULL, 2, &CommsTaskHandle);

	// Start the FreeRTOS Scheduler
	CA_OS_Init();
	vTaskStartScheduler();

	// Should never get here
	for (;;)
	{
	}
}

/* Stack overflow hook, required by FreeRTOS. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	/* Force an assert. */
	configASSERT(pcTaskName == 0);
}
