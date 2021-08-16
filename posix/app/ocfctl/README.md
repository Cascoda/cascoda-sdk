# ocfctl

`ocfctl` is a terminal application used to control a Chili device programmed with an OCF target such as `ocf-light`. The main purpose of this application is to provide a convenient way of interacting with the device during OCF certification whilst using as few resources as possible on the embedded device.

## Basic Usage

Flash a Chili2D with an `ocf-light` binary that has been configured with the USB interface. Then, connect the Chili to a computer via USB and run `ocfctl` on the host computer. On first launch, `ocfctl` will display information about the connected device:

```
Rx: 1ms NOTE: EVBME connected, ocf-light, v0.17-11-geea8d7d5-dirty Apr 29 2021
Rx: 2ms NOTE: Device ID: 588cac70a197e8f5
Rx: 2322ms: [NOTE]-MLE-----: Role Disabled -> Detached
Rx: Used input file : "../iotivity-lite-lightdevice/out_codegeneration_merged.swagger.json"
Rx: OCF Server name : "server_lite_53868"
Rx: Intialize Secure Resources
Rx:      introspection via header file
Rx: Register Resource with local path "/binaryswitch"
Rx:      number of Resource Types: 1
Rx:      Resource Type: "oic.r.switch.binary"
Rx:      Default OCF Interface: 'oic.if.a'
Rx: Register Resource with local path "/dimming"
Rx:      number of Resource Types: 1
Rx:      Resource Type: "oic.r.light.dimming"
Rx:      Default OCF Interface: 'oic.if.a'
Rx: OCF server "server_lite_53868" running, waiting on incoming connections.
```

Depending on the log level setting that the binary was configured with, `ocfctl` may display additional information which is useful for debugging.

You control the Chili by typing commands and hitting the Enter key. Here is a list of currently supported commands:

### rfotm

The `rfotm` command resets the OCF data of the device, clears its owner & puts it in the Ready for Ownership Transfer Method state. This command should be entered when you are instructed to do so by OCTT, or when you want to reset the ownership data of an OCF device for any other reason.

After executing this command, you should see messages describing the installation of the default PKI certificates.

```
rfotm
Rx: Successfully installed PKI certificate
Rx: Successfully installed intermediate CA certificate
Rx: Successfully installed root certificate
Rx: Successfully installed PKI certificate
Rx: Successfully installed intermediate CA certificate
Rx: Successfully installed root certificate

```

### power

The `power` command resets the microcontroller & radio transceiver exactly as if they had been power cycled. This command can be used whenever OCTT instructs you to power-cycle the device. Alternatively, you may unplug it from the host computer, plug it back in and re-open `ocfctl`.

Upon running the `power` command, you should see a message about the HID device being dropped, followed by the device information usually shown when connecting for the first time.

```
power
2021-04-29 16:07:25.901 WARN:  Hid device dropped... attempting reload
Rx: 1ms NOTE: EVBME connected, ocf-light, v0.17-11-geea8d7d5-dirty Apr 29 2021
Rx: 2ms NOTE: Device ID: 588cac70a197e8f5
Rx: 2322ms: [NOTE]-MLE-----: Role Disabled -> Detached
Rx: Used input file : "../iotivity-lite-lightdevice/out_codegeneration_merged.swagger.json"
Rx: OCF Server name : "server_lite_53868"
Rx: Intialize Secure Resources
Rx:      introspection via header file
Rx: Register Resource with local path "/binaryswitch"
Rx:      number of Resource Types: 1
Rx:      Resource Type: "oic.r.switch.binary"
Rx:      Default OCF Interface: 'oic.if.a'
Rx: Register Resource with local path "/dimming"
Rx:      number of Resource Types: 1
Rx:      Resource Type: "oic.r.light.dimming"
Rx:      Default OCF Interface: 'oic.if.a'
Rx: OCF server "server_lite_53868" running, waiting on incoming connections.

```

## Technical Detail

`ocfctl` works by sending [asynchronous TLV (type-length-value) user commands](../../../../docs/reference/cascoda-tlv-message.md) to the connected device. Command code `0xB0` is used for all `ocfctl` TLV commands. The length byte must be set to 1, and the value describes the specific `ocfctl` command sent to the Chili. For instance, `B0 01 00` is used for the `rfotm` command, and `B1 01 01` is used for the `power` command.

To keep things simple, and to avoid concurrency bugs, `ocfctl` only _sends_ custom commands to the device, it does not receive any. The only information received by `ocfctl` from the Chili consists of EVBME messages used to display device information & debugging logs.

On the embedded device, the commands are implemented in `ocf/apps/wakeful_main.c`.
