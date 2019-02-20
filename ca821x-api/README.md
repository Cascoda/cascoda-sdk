# ca821x-api
This is Cascoda's IEEE 802.15.4 API library used for communication with the CA-821X family of devices.

The API models the SAP interface of the 802.15.4 specification (MCPS+MLME) as well as two proprietary entities, the HWME (Hardware Management Entity) and the TDME (Test & Debug Management Entity). The API defines two main types of command; synchronous and asynchronous. Synchronous command functions will not return until the corresponding response has been received from the device whereas asynchronous functions will return immediately after the command is issued.

## Building
This project must be configured with cmake before building. It is recommended to use the cmake gui, ccmake or cmake interactive mode, as there are user-configurable options. This will generate the required config files and setup whatever build system/projects you need.

For more information, consult https://cmake.org/runningcmake/

## Usage

Before using the api, a ca821x_dev struct must be allocated and initialised.
```C
struct ca821x_dev pDeviceRef;
ca821x_api_init(&pDeviceRef);
```

A set of callbacks is provided for processing asynchronous responses:
```C
struct cascoda_api_callbacks {
	//...
}
```
An instance of this struct must be created in the user's application and registered using the function
```C
int cascoda_register_callbacks(struct cascoda_api_callbacks *in_callbacks, struct ca821x_dev *pDeviceRef);
```

All downstream commands will be sent using the function pointer:
```C
extern int (*ca821x_api_downstream)(
	const uint8_t *buf,
	size_t len,
	uint8_t *response,
	struct ca821x_dev *pDeviceRef
);
```
This pointer must be populated with an implementation conforming to this prototype. The function should transmit the contents of `buf` to the CA-821X device and populate `response` with whatever synchronous response is received (if `buf` contains a synchronous command). If `buf` contains an asynchronous command, `response` can be ignored.<br>
`pDeviceRef` is passed through to this function from the API call at the top level. It can be used to identify the CA-821X instance being controlled (e.g. passing a private data reference, device ID etc). The 'void *context' data member of the struct is reserved for application usage.

The API is based off of 802.15.4-2006, please see the IEEE specification and the Cascoda datasheets [CA-8210](http://www.cascoda.com/wp/wp-content/uploads/CA-8210_datasheet_1016.pdf) (section 5) for more detailed information.
