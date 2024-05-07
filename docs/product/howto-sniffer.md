# Getting Started with the Cascoda Packet Sniffer

The Packet Sniffer is a tool for capturing IEEE 802.15.4 traffic. Using Wireshark, it can decode and analyze Thread, KNX-IoT and Zigbee packet headers.

## Setup

### 1. Get a Cascoda Packet Sniffer dongle

[The Packet Sniffer dongle is available from our distributors](https://www.cascoda.com/wheretobuy/). It consists of a [Chili2D module](https://www.cascoda.com/products/module/chili2d/) pre-programmed with dedicated packet sniffing firmware. 

### 2. Install the Cascoda Windows Tools

[Get the Cascoda Windows Tools installer](https://github.com/Cascoda/cascoda-sdk/releases) from our Releases page, and install it so that you have access to the sniffer executable.

### 3. Install Wireshark

[Install Wireshark from the official website](https://www.wireshark.org/download.html), if it is not installed already. 

### 4. Run the sniffer

Plug in your Packet Sniffer dongle, and run the sniffer executable. To start a live capture on channel 15, you must open the Command Prompt and type `sniffer -w 15`.

## Packet Sniffer Reference

In order to decrypt Thread frames successfully, you will have to inform Wireshark of the Thread network decryption key. If you have the KNX-IoT access tokens, OSCORE security can also be decrypted here. [See our sniffer documentation](/posix/app/sniffer/README.md#configuring-wireshark) for detailed instructions.