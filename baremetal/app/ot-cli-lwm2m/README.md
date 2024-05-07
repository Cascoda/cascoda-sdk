# ot-cli-lwm2m

This example application demonstrates a LWM2M 1.1 client running on a Thread node.
It uses the OpenThread CLI to drive the demonstration, with custom commands to drive the LWM2M functionality.

Further information about LWM2M can be found in the [LWM2M Core Specification](http://www.openmobilealliance.org/release/LightweightM2M/V1_1-20180710-A/OMA-TS-LightweightM2M_Core-V1_1-20180710-A.pdf)
and the [LWM2M Transport Specification](http://www.openmobilealliance.org/release/LightweightM2M/V1_1-20180710-A/OMA-TS-LightweightM2M_Transport-V1_1-20180710-A.pdf).
This example includes support for the CoAP over UDP and CoAP over DTLS transport bindings.

In order to run this demonstration, a Thread border router, such as the Cascoda evaluation border router is required. 
The border router should be upgraded to the latest version using the scripts available [here](https://github.com/Cascoda/install-script).

Please see the [evaluation guide](../../../docs/dev/lwm2m-over-thread.md) for more information on how to use this binary.
The guide also includes some [considerations](../../../docs/dev/lwm2m-over-thread.md#considerations) for use in practice.

**Note that if secure LWM2M is required (using DTLS), the ``CASCODA_BUILD_SECURE_LWM2M`` must be enabled in the CMake config.**

## Commands

In addition to the standard OpenThread CLI commands, which are documented [here](https://github.com/Cascoda/openthread/tree/ext-mac-dev/src/cli), the following are also implemented, as extracted from the [wakaama demo](https://github.com/eclipse/wakaama).

- [lwstart](#lwstart)
- [lwstop](#lwstop)
- [lwlist](#lwlist)
- [lwchange](#lwchange)
- [lwupdate](#lwupdate)
- [lwls](#lwls)
- [lwrm](#lwrm)
- [lwadd](#lwadd)
- [lwdisp](#lwdisp)
- [lwdispb](#lwdispb)


### lwstart

Start the LWM2M Client stack, bootstrapping or directly connecting to a server as required. This function takes arguments in POSIX style.

| Option       | Description |
| ------------ | ----------- |
|-n NAME       | Set the endpoint name of the Client. Default: testlwm2mclient
|-l PORT       | Set the local UDP port of the Client. Default: 0 (ephemeral)
|-h HOST       | Set the hostname of the LWM2M Server to connect to.
|-p PORT       | Set the port of the LWM2M Server to connect to. Default: 5683 for ``coap://``, 5684 for ``coaps://``
|-t TIME       | Set the lifetime of the Client in seconds. Default: 300s
|-b            | Bootstrap requested.
|-c            | Change battery level over time.
|-i STRING     | Set the device management or bootstrap server PSK identity for PSK security. Example: ``-i examplepskidentity``
|-s HEXSTRING  | Set the device management or bootstrap server Pre-Shared-Key for PSK security. If not set use non-secure mode. Example: ``-s aabbccddeeff0011223344556677889900``

#### Examples:

Start the LWM2M client without security, name 'cascoda-client', connecting to a server running on the adjacent LAN network:

``lwstart -n cascoda-client -h fdaa:bbcc:ddee:0:a196:ec9a:66bc:f69``

_Note: Running without security - of course - offers no end-to-end security, so is a poor choice for almost all applications._

---

Start the LWM2M client, with endpoint name 'cas-test', connecting to the server at 'leshan.eclipseprojects.io' with identity `0123456789` and preshared secret key `00112233445566778899`

``lwstart -n cas-test -h leshan.eclipseprojects.io -i 0123456789 -s 00112233445566778899``

_Note: The identity and PSK must be pre-configured on the server or this will fail with security error._

--- 

Start the LWM2M client, with endpoint name 'bootstrap-test', connecting to the server at the given IPv6 address (Server running on local network in this case), port 5784, identity `9879879878`, preshared secret key `aabbcc334488227755009911`:

``lwstart -b -n bootstrap-test -h fdaa:bbcc:ddee:0:a196:ec9a:66bc:f69 -p 5784 -i 9879879878 -s aabbcc334488227755009911``

_Note: The identity and PSK must be pre-configured on the bootstrap server, and the bootstrap server must be configured to bootstrap the client with URI and credentials that are valid for the server that it will register to._

### lwstop

Stop the LWM2M client from running.

### lwlist

List the configured servers and their states.

Example:
```
> lwlist
Bootstrap Servers:
 - Security Object ID 0 Hold Off Time: 1 s      status: DEREGISTERED
LWM2M Servers:
 - Server ID 123        status: Rx: REGISTERED  location: "/rd/yUo48kLNRY"      Lifetime: 300s
```

### lwchange

Change (or register a change) of a value in a local object. This can be used to update values that are being 'observed' by servers.

Example, reporting a change without value (here we trigger a notification to observers of the 'time' resource):
```
> lwchange /3/0/13
Rx: report change!
```

Example, changing a value and reporting it to watchers (here we update the battery percentage to 90%):
```
> lwchange /3/0/9 90
Rx: value changed!
```

### lwupdate

Trigger a server registration update for the given server now.

Example:

```
> lwupdate 123
```

### lwls 

List the available object instance URIs.

Example:

```
> lwls
/0/0  /0/1
/1/0
/2/0
/3/0
/4/0
/5/0
/6/0
/7/0
/31024/10  /31024/11  /31024/12
```

### lwrm

Remove the test object at ``/31024`` (used to demonstrate dynamic objects)

### lwadd

Re-add the test object at ``/31024`` after removing it.

### lwdisp

Display objects (pulled from wakaama demo, not fully functional).

Example:
```
> lwdisp
Rx:   /0: Security object, instances:
Rx:     /0/0: instanceId: 0, uri: coaps://127.0.0.1:5784, isBootstrap: true, shortId: 111, clientHoldOffTime: 1
Rx:     /0/1: instanceId: 1, uri: coaps://[fdaa:bbcc:ddee:0:a196:ec9a:66bc:f69]:5684, isBootstrap: false, shortId: 123, clientHoldOf
Rx: fTime: 1
Rx:   /1: Server object, instances:
Rx:     /1/0: instanceId: 0, shortServerId: 123, lifetime: 300, storing: true, binding: U
Rx:   /3: Device object:
Rx:     time: 1612527196, time_offset: +01:00
Rx:   /5: Firmware object:
Rx:     state: 1, result: 0
Rx:   /6: Location object:
Rx:     latitude: , longitude: , altitude: , radius: , timestamp: 1612517584, speed:
Rx:   /31024: Test object, instances:
Rx:     /31024/10: shortId: 10, test: 20
Rx:     /31024/11: shortId: 11, test: 21
Rx:     /31024/12: shortId: 12, test: 22
```

### lwdispb

Display the security and server objects that are backed up during bootstrapping, in case of failure.

Example:
```
>lwdispb
Rx:   /0: Security object, instances:
Rx:     /0/0: instanceId: 0, uri: coaps://fdaa:bbcc:ddee:0:a196:ec9a:66bc:f69:5784, isBootstrap: true, shortId: 123, clientHoldOffTi
Rx: me: 10
Rx:   /1: Server object, instances:
Rx:     /1/0: instanceId: 0, shortServerId: 123, lifetime: 300, storing: false, binding: U
>
```

---
_Copyright (c) 2021 Cascoda Ltd._