# ca821x-api
This is Cascoda's IEEE 802.15.4 API library used for communication with the CA-821X family of devices.

The API models the SAP interface of the 802.15.4 specification (MCPS+MLME) as well as two proprietary entities, the HWME (Hardware Management Entity) and the TDME (Test & Debug Management Entity). The API defines two main types of command; synchronous and asynchronous. Synchronous command functions will not return until the corresponding response has been received from the device whereas asynchronous functions will return immediately after the command is issued.

The API is based off of 802.15.4-2006, please see the IEEE specification and the Cascoda datasheets [CA-8210](https://www.cascoda.com/wp-content/uploads/2018/11/CA-8210_datasheet_0418.pdf) & [CA8211](https://www.cascoda.com/wp-content/uploads/2019/01/CA-8211_datasheet_0119.pdf) (section 5) for more detailed information.

## Application Usage

Before using the api, a ca821x_dev struct must be allocated and initialised.
```C
struct ca821x_dev pDeviceRef;
ca821x_api_init(&pDeviceRef);
```

A set of callbacks exist in the ca821x_dev struct for processing asynchronous responses:
```C
	pDeviceRef->callbacks.MCPS_DATA_indication;
	pDeviceRef->callbacks.MLME_SCAN_confirm;
	pDeviceRef->callbacks.HWME_WAKEUP_indication;
	pDeviceRef->callbacks.generic_dispatch;
	//etc...
```
These callbacks should be populated by the application as required, or can be NULL if they are not required. The generic_dispatch callback is called if the relevant callback for the received command isn't populated, so can be useful for debugging.

The pDeviceRef also includes a ```void *context``` pointer for use by the application, which can be used for whatever the application requires. It is useful for when a single program is using multiple devices with multiple application states at the same time.


### Header files

The header file ```ca821x_api.h``` includes all of the core functionality of the api, including all MCPS, MLME, TWME & HWME functions. It also includes the api_init function and some others that may potentially be useful.

The header file ```ca821x_api_helper.h``` includes a collection of helper functions that are useful for reading and writing to some of the data structures that have variable length components, such as MLME_Scan.confirms and Key Table Entries.

The header file ```mac_messages.h``` includes some useful struct definitions for interacting with the API.

The header files ```ca821x_endian.h, ca821x_error.h, hwme_tdme.h, ieee_802_15_4.h``` provide some useful enumerations and macro functions.

## Exchange (Platform abstraction) Usage

All downstream commands will be sent using the function pointer in the pDeviceRef:
```C
int (*ca821x_api_downstream)(
	const uint8_t *buf,
	size_t len,
	uint8_t *response,
	struct ca821x_dev *pDeviceRef
);
```
This pointer must be populated with an implementation conforming to this prototype. The function should transmit the contents of `buf` to the CA-821X device and populate `response` with whatever synchronous response is received (if `buf` contains a synchronous command). If `buf` contains an asynchronous command, `response` can be ignored.<br>
`pDeviceRef` is passed through to this function from the API call at the top level. It can be used to identify the CA-821X instance being controlled (e.g. passing a private data reference, device ID etc). The 'void *context' data member of the struct is reserved for application usage.
