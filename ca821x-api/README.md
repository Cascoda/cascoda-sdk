# ca821x-api

This is Cascoda's IEEE 802.15.4 API library used for communication with the CA-821X family of devices.

The API models the SAP interface of the 802.15.4 specification (MCPS+MLME) as well as two proprietary entities, the HWME (Hardware Management Entity) and the TDME (Test & Debug Management Entity). The API defines two main types of command:

- **Synchronous**. Synchronous functions are blocking, and will not return until the corresponding response has been received from the device
- **Asynchronous**. Asynchronous functions will return immediately after the command is issued.

The API also defines a mechanism to handle 'indications' which arrive from the CA-821x without a requesting command, such as the MCPS Data Indication for received data. This is also used for Asynchronous responses - see the ``ca821x_api_callbacks`` struct.

The API is based on IEEE 802.15.4-2006, please see the IEEE specification and the Cascoda datasheets [CA-8210](https://www.cascoda.com/wp-content/uploads/2018/11/CA-8210_datasheet_0418.pdf) & [CA8211](https://www.cascoda.com/wp-content/uploads/2019/06/CA-8211_datasheet_0119.pdf) (section 5) for more detailed information.

## Application Usage

Before using the api, a ca821x_dev struct must be allocated and initialised.

```C
struct ca821x_dev pDeviceRef;
ca821x_api_init(&pDeviceRef);
```

A set of callbacks exist in the ca821x_dev struct for processing asynchronous responses and indications:

```C
 pDeviceRef->callbacks.MCPS_DATA_indication;
 pDeviceRef->callbacks.MLME_SCAN_confirm;
 pDeviceRef->callbacks.HWME_WAKEUP_indication;
 pDeviceRef->callbacks.generic_dispatch;
 //etc...
```

These callbacks should be populated by the application as required, or can be NULL if they are not required. The generic_dispatch callback is called if the relevant callback for the received command isn't populated, so can be useful for debugging.

The pDeviceRef also includes a ```void *context``` pointer for use by the application, which can be used for whatever the application requires. It is useful for when a single program is using multiple devices with multiple application states at the same time.

## Exchange (Platform abstraction) Usage

All downstream commands will be sent using the ``ca821x_api_downstream`` function, which is implemented seperately for posix and baremetal systems.

The function transmits the contents of `buf` to the CA-821X device and populates `response` with whatever synchronous response is received (if `buf` contains a synchronous command). If `buf` contains an asynchronous command, `response` can be ignored.

`pDeviceRef` is passed through to this function from the API call at the top level. It can be used to identify the CA-821X instance being controlled (e.g. passing a private data reference, device ID etc). The 'void *context' data member of the struct is reserved for application usage.
