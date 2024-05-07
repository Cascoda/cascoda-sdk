# How to configure a Cascoda Border Router to use a USB dongle

By default, the border router is configured to use the UART exchange for its Thread communications. This is accomplished through the `CASCODA_UART` environment variable, which is set within `/etc/init.d/otbr-agent`.

If the `CASCODA_UART` variable is not set, the exchange will look for a Chili connected via USB, which [must be flashed](../dev/flashing.md) with the `mac-dongle` binary configured with an USB exchange - [the one within the Chili2D-USB release will do](https://github.com/Cascoda/cascoda-sdk/release).

## Steps

1. Within the Cascoda border router, open `/etc/init.d/otbr-agent` configuration file using `vim` and comment out the line that sets the `CASCODA_UART` variable. Your config file should now look like this:

```bash
START=90

USE_PROCD=1

start_service() {
        procd_open_instance
        mkdir -p /run/
        sleep 4
        #procd_set_param env CASCODA_UART=/dev/ttyS0,115200
        procd_set_param command /usr/sbin/otbr-agent -I wpan0 -B br-lan 'spinel+hdlc+uart://irrelevant-path'
        procd_close_instance
}
```

2. Shut down the Border Router using `poweroff`, then disconnect the power supply after waiting for several seconds.

3. After the power supply has been disconnected, connect a Chili2D USB dongle flashed with the `mac-dongle` binary.

4. Connect the Border Router power supply and check that `otbr-agent` was initialized correctly and is able to handle Thread messages. You can do so with the `logread | less` command. Finally, you can verify that the higher layers of OpenThread are functionally correctly using the `ip address show dev wpan0` command - you should see multiple IPv6 addresses currently in use by Thread.
