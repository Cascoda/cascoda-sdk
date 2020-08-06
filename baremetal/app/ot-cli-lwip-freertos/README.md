# LwIP FreeRTOS demo #

This demonstration shows LwIP capabilities in combination with freeRTOS. This enables the posix-style Socket API, which means that existing posix applications can be easily ported to run on the Chili2 and Thread. This demo is compatible with ot-cli-lwip.

This demo provides the standard openthread CLI as documented here: https://github.com/Cascoda/openthread/tree/ext-mac-dev/src/cli

The additional 'lwip' command can be used to open and send data over TCP connections:

```
> lwip
> lwip - print help
lwip tcp - print status
lwip tcp con <ip> - open tcp connection to port 51700
lwip tcp sen <msg> - send text string over tcp connection
lwip tcp clo - close tcp connection
lwip dns <hostname> - Resolve IP address of hostname using DNS
lwip dns ser <ipv6 addr> - Set the IPv6 address of the DNS server
```

The demo is listening on port 51700 for incoming connections, and will print any data received on this port as text.
