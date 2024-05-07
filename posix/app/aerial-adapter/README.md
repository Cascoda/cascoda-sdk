# aerial-adapter

`aerial-adapter` is a bash shell script for connecting to a target device without USB or UART serial interface. It starts up a reference device which is opening an over-air communications channel with the target device, sets up the communications channel and invokes serial-adapter for communications.

Usage:
```
aerial-adapter channel [serialnr]
   channel   IEEE802.15.4 channel to be used (11-26)
   serialnr  serial number of the reference device (optional)

For example:

aerial-adapter 15 7572CD4ACEDEA95A
```

The reference device has to run the baremetal production test application binary `testref` (the source code can be found in baremetal/app/testref).

For the target device only few additions are required to allow the use of printf style functions over air for debugging etc.:

Add the library `cascoda-aerial-adapter` in the cmake target link library list of your application.
```
target_link_libraries(...  cascoda-aerial-adapter ...)
```

Include file:
```
    #include "cascoda-bm/cascoda_aerial_adapter.h"
```

Initialise aerial-adapter with initialisation function after startup (after EVBMEInitialise(), i.e. using channel 15):
```
	aainitialise(15, &dev);
```

The aerial-adapter aaprintf can then be used with the same style format strings and arguments as a normal printf, i.e.:
```
	aaprintf("Hello World!");
```

The aaprintf message length is limited to 100 characters as over-air fragmentation is not supported. Strings longer than this will be truncated.

The channel used by aerial-adapter should differ from the network channel when running a Thread or Zigbee application. The PAN Information Database (PIB) of the application will be saved before switching the channel to the aerial-adapter channel for aaprintf, and will be restored after the communication.

For applications using Thread the EVBME radio re-initialisation callback cascoda_reinitialise should be registered and call otLinkSyncExternalMac() in order to restore the security part of the PIB (in the same way as it is done for sleepy devices).

Note that aaprintf should not be used in registered API callback functions when using WAIT_Callback() or WAIT_CallbackWithRef() to synchronise to API confirms or indications as the functions are used in aerial-adapter and are strictly not re-entrant. If used the aerial-adapter interface will be disabled, and aaprintf will return with an error (-1). The use of aaprintf should also be avoided when over-air packets are to be immediately expected, as the change of channel will temporarily interrupt network traffic for the device.
