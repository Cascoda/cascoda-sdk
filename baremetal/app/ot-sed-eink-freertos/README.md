# Sleepy End Device EPaper demo #

This target contains an application which requests compressed images from a
server (`ot-eink-server`) and displays them onto a Waveshare 2.9" e-Paper
module. The pin connections between the e-Paper module and the Chili are shown in the table below:

<table>
<thead>
  <tr>
    <th><span style="font-weight:bold">EINK</span></th>
    <th><span style="font-weight:bold">CHILI</span></th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>BUSY</td>
    <td>PIN 31</td>
  </tr>
  <tr>
    <td>RST</td>
    <td>PIN 15</td>
  </tr>
  <tr>
    <td>DC</td>
    <td>PIN 34</td>
  </tr>
  <tr>
    <td>CS</td>
    <td>GDN</td>
  </tr>
  <tr>
    <td>CLK</td>
    <td>PIN 33</td>
  </tr>
  <tr>
    <td>DIN</td>
    <td>PIN 32</td>
  </tr>
  <tr>
    <td>GDN</td>
    <td>PINS 3/14/16/18-25/27/30</td>
  </tr>
  <tr>
    <td>VCC</td>
    <td>PIN 13</td>
  </tr>
</tbody>
</table>

See [the server documentation](../../../posix/app/ot-eink-server/README.md) for a high
level overview of the application as a whole, and for a description of how to
generate images compatible with this application.

## How to run & test

This demonstration requires another Chili to act as the server. Flash the
server Chili with the `mac-dongle` binary. You must also build the
`ot-eink-server` POSIX application, from within the Cascoda POSIX SDK
build directory.

Once the devices are all ready, plug the server Chili into a Linux machine
and run the `ot-eink-server` executable. Then, power the end device
Chili and [commission it](../../../docs/guides/thread-commissioning.md). After 10 to 20 seconds, the server will receive discover requests, as
well as image requests from the end device.
