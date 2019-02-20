# SPI Driver Overview
One component of this project is an SPI master driver. Please see the [CA-8210 datasheet](https://www.cascoda.com/wp/wp-content/uploads/CA-8210_datasheet_0418.pdf) for more detailed information on the SPI protocol.

This SPI driver operates in full-duplex mode and makes a distinction between synchronous and asynchronous message exchanges.

Synchronous messages will always be paired. After a command is sent from the master to the slave, the master will wait until the corresponding response message is received from the slave (or until a timeout period \ref SPI_T_TIMEOUT has elapsed). This behaviour is implemented by the \ref SPI_Send function.

Asynchronous commands are transmitted in isolation, and responses may be received at any time. Determining when to attempt to read these asynchronous messages from the slave is not the responsibility of the SPI driver, as it requires platform-specific support (i.e interrupt handling). Every time a new message is received from the slave, it should be stored in the next available index of \ref SPI_Receive_Buffer.