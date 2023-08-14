<p align="center"><img src="etc/img/cascoda.png" width="75%"></p>

# What is the Cascoda SDK?

The Cascoda Software Development Kit ...

...**includes a comprehensive set of tools for developing systems integrating [Cascoda hardware](https://www.cascoda.com/products/).** The SDK is designed to be cross-platform and flexible, enabling designing on one system and porting to another with ease. Many [example applications](docs/reference/full-reference.md#example-applications) are included in order to demonstrate use of the systems.

...**contains a general API to interact with the [CA-8210](https://www.cascoda.com/CA-8210/) or [CA-8211](https://www.cascoda.com/CA-8211/), which can be run on Bare Metal or Linux systems.** It also contains a collection of custom and third-party helper libraries, which have been integrated for development convenience. A complete reference of the Cascoda SDK API can be found [here](https://cascoda.github.io/cascoda-sdk-doxygen/).

...**contains a Bare Metal board support package (BSP) which provides a portable abstraction for Bare Metal platforms**, and a useful set of libraries that can be used to bootstrap development. The Linux platform takes advantage of the extra functionality to enable control of multiple devices at a time, and dynamic selection of SPI/UART/USB Cascoda devices.

...**contains Thread applications, which when run on the [Chili2D](https://www.cascoda.com/chili2D/), result in a Thread Certified Component, proving compliance with the Thread 1.1 specification.** (Note: this only applies to the SDK [version 0.13](https://github.com/Cascoda/cascoda-sdk/releases/tag/v0.13-2)). This means that developers can use the Chili2 and Cascoda SDK in confidence to develop a quality Thread Product. It also opens up the path of 'Certification by Inheritence' at the Thread Group [Implementer Tier](https://www.threadgroup.org/thread-group) of membership.

![Thread Certified Component Logo](etc/img/thread_certified_component.png)

# Getting started

We offer binaries that can be flashed directly onto our Chili2D USB module
inside `Chili2D-USB.zip`. Cascoda's Windows Tools are provided via an installer ([click here to download](https://github.com/Cascoda/cascoda-sdk/releases/latest)). However, `Chili2S-UART0-1Mbaud.zip` is _for advanced users
only_ as these binaries _cannot be used with our USB module_, only with Chili2S
devices soldered onto a PCB.

[**Click here to for the latest release of all binaries and tools mentioned above.**](https://github.com/Cascoda/cascoda-sdk/releases)

A reference of all documentation and guides is located [here](docs/reference/full-reference.md), but to get started, here is a list of things you might be looking to do...

- [Getting started with the Cascoda KNX IoT Dev Kit](docs/how-to/howto-knxiot-devkit.md)
- [Getting Started with the Cascoda Packet Sniffer](docs/how-to/howto-sniffer.md)
- [Build, configure and set up a Thread network](docs/how-to/howto-thread.md)
- [Build embedded applications to communicate with various sensors and actuator](docs/how-to/howto-devboard.md)
- [Learn how to build the Cascoda SDK](docs/reference/full-reference.md#building)

If you're a developer or simply want to delve deeper into the Cascoda SDK, read on.

# For developers

[The reference document for the Cascoda SDK](docs/reference/full-reference.md) is a great place to get started if you want to delve deeper into the Cascoda SDK! In that document, you will find among other things...

- Development tips: guides on building, configuring, flashing and debugging
- Details about the internal workings of the different modules, APIs, and applications
- List of all the tools and applications, both for embedded and hosted platforms
- Description of the architecture and layout of the SDK, and what is available in each folder

# Need help?

If you want to report bugs or request features, submit your request to the [Issue Tracker](https://github.com/cascoda/cascoda-sdk/issues).

# Security information

Information about security vulnerabilities can be found [here](SECURITY.md).