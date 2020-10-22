# Sniffer

An example program for sniffing 802.15.4 traffic on a specific channel.

It can run on Windows or Posix and can:
- Capture to stdout
- Output to a pcap file
- Stream directly to wireshark/tshark

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
