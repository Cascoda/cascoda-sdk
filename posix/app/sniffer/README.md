# Sniffer

A hosted program for sniffing IEEE 802.15.4 traffic on a specific channel.

The sniffer uses a Chili2D to passively sniff radio traffic. The sniffed radio traffic can be decoded by Wireshark and interpreted as Thread, IPv6, CoAP or DTLS, based on its contents.

Prebuilt Windows binaries of the sniffer can be found in the [Windows release of the Cascoda SDK.](https://github.com/Cascoda/cascoda-sdk/releases/)

## Table of Contents
- [Sniffer](#sniffer)
  - [Table of Contents](#table-of-contents)
  - [Quick Start](#quick-start)
  - [Configuring Wireshark](#configuring-wireshark)
  - [Detailed Explanation](#detailed-explanation)


## Quick Start

Connect a Chili2D running the USB `mac-dongle` binary to your computer.

On a desktop computer, use the following terminal command to start a sniffer on channel 21, and open a Wireshark window to view the sniffed packets. This is the most common usecase and it works on Windows & Linux desktops.
```bash
./sniffer 21 -w
```

Alternatively, when using a headless computer, you can start a sniffer on channel 21, output to stdout in the .pcap format & pipe to tshark which is receiving on stdin.
```bash
./sniffer 21 -p | tshark -i -
```

On headless computers it may also useful to start a sniffer on channel 21 & save to a file called capture.pcap, which can later be opened by Wireshark.
```bash
./sniffer 21 -p > capture.pcap
```

## Configuring Wireshark

Before Wireshark can decode secured Thread traffic, the master key of the Thread network must be entered. Please navigate to Edit -> Preferences, expand the Protocol tab in the left-hand side of the pop-up window and scroll down to the "IEEE 802.15.4" protocol. Hit the Edit button next to Decryption Keys, and then hit the plus button to add a new key. After pasting in your key, ensure that the "Key hash" value of the key entry is set to "Thread hash", so that the newly made key entry is used for Thread networks. Hit "Ok" on all windows to save your work.

Here is an example configuration where two Thread key entries have been added:
![Wireshark thread keys window, showing two correctly configured Thread master keys](correctly-configured-keys.png)

After a correct master key has been input, 15.4 traffic should automatically be decoded as Thread - you should be able to see the 6LoWPAN headers on most packets. Some application-layer protocols such as DNS, NTP, CoAP are also decoded.

Occasionally, Wireshark fails to detect the high level protocol used in some packets. These will show up as plain UDP packets with somewhat large payloads. If you know the protocol you are expecting to decode, you can right-click on these packets and select "Decode As...". Then, edit the Current field of the "UDP port" entry at the bottom, and select the protocol you are expecting to decode. For OCF over Thread, this is usually CoAP or DTLS.

## Detailed Explanation

The Cascoda sniffer can run on Windows or Posix and can:
- Capture to stdout
- Output to a pcap file
- Stream directly to wireshark/tshark

The sniffer does not _directly_ participate in Thread or 802.15.4 networks: it is a passive device that cannot transmit any packets. It is able to decode Thread traffic by intercepting packets within its reception range and decrypting the payload using a user-supplied masterkey.

Run ./sniffer with no args to print the help page.

```
sniffer.exe [OPTIONS] CHANNEL
        Print all received packets on channel (11-26)

DESCRIPTION
        Sniffer program to use the CA-8211 to capture packets on a channel.

        -p             PCap mode, output pcap data instead of descriptive hex
                       dump. If this is used in conjunction with pipes, can be
                       used to stream to wireshark

        -d             Debug mode, print verbose information to stderr.

        -n [PIPENAME]  Output to a named pipe/fifo, which can be read by
                       wireshark or another program. Most useful in conjunction
                       with '-p'.

        -w             Open WireShark to process the packet capture. Implies -p
                       and -n (random name for pipe if not provided separately).

        -W [PATH]      Open WireShark at the path to process the packet capture.
                       Implies -w.
```

For instance the ``-w`` argument can be used to automatically boot wireshark and connect. On unix platforms the usage is more flexible and can use command line pipes such as:

```bash
# Start sniffer on channel 21, pcap output, pipe to tshark, which is receiving on stdin.
./sniffer 21 -p | tshark -i -
```
