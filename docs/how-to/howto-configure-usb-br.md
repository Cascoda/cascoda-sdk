# How to configure a Cascoda Border Router to use a USB dongle

By default, the setup script configures the border router to use the UART
exchange for its Thread communications. This is accomplished through the
`CASCODA_UART` environment variable, which is set within `/etc/default/wpantund`

If the `CASCODA_UART` variable is not set, the exchange will look for a Chili
connected via USB, which must be flashed with the `mac-dongle` binary configured
with an USB exchange - the one within the Chili2D-USB release will do.

## Steps

1. Within the Cascoda border router, open `/etc/default/wpantund` configuration
file in your text editor of choice and comment out the line that sets the
`CASCODA_UART` variable. Your config file should now look like this:

```bash
WPANTUND_OPTS=-o Daemon:SetDefaultRouteForAutoAddedPrefix true -o IPv6:SetSLAACForAutoAddedPrefix true
# CASCODA_UART=/dev/serial0,1000000
```

2. Shut down the Raspberry Pi using `sudo shutdown now`, then disconnect the
power supply after waiting for several seconds.

3. After the power supply has been disconnected, remove the Chili2S PiHat and
connect a Chili2D USB dongle flashed with the `mac-dongle` binary.

4. Connect the Raspberry Pi power supply and check that `wpantund` was
initialized correctly. You can do so with the `systemctl status
wpantund.service` command. Initialization of the NCP should succeed, and the
state should be `leader` or `router`. If there are errors, you can view the full
logs using `journalctl -u wpantund.service`. Finally, you can verify that the
higher layers of OpenThread are functionally correctly using the `ip address
show dev wpan0` command - you should see multiple IPv6 addresses currently in
use by Thread.
