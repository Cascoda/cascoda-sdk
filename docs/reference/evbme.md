# Evaluation Board Management Entity

The Evaluation Board Management Entity (EVBME) is the software component present in every baremetal firmware. It is the
API endpoint for all things related to the software running on a Chili module, and is also responsible for bridging the
communications between a host OS and the MLME (Mac SubLayer Management Entity), MCPS (Mac Common Part Sublayer) and HWME
(HardWare Management Entity).

The EVBME is extendable per application to include additional commands, but every application which has a serial interface
will support the following, which are supported by API calls on both host and embedded platforms.

For exact message structures, see the ``evbme_messages.h`` header file.

| Command                    | ID   | Direction | Synchronous? | Description
| -------------------------- | ---- | --------- | ------------ | -----------
| EVBME Get Request          | 0x5D | H ->  C   | Sync  | Get an EVBME parameter (sync req)
| EVBME Get Confirm          | 0x5C | H <-  C   | Sync  | Response containing EVBME parameter data
| EVBME Set Request          | 0x5F | H ->  C   | Async | Set an EVBME parameter
| EVBME Set Confirm          | 0x5E | H <-  C   | Async | Response including status for EVBME set
| EVBME Host Connected       | 0x81 | H ->  C   | Async | Notification from host that connection is established
| EVBME Host Disconnected    | 0x82 | H ->  C   | Async | Notification from host that connection is about to be terminated
| EVBME Message Indication   | 0xA0 | H <-  C   | Async | Text message to be printed by host
| EVBME Comm Check           | 0xA1 | H ->  C   | Async | Communication check message from host that generates COMM_INDICATIONS
| EVBME Comm Indication      | 0xA2 | H <-  C   | Async | Communication check indication from slave to master as requested
| EVBME DFU                  | 0xA3 | H <-> C   | Async | DFU Commands for Device Firmware Upgrade in system. See [DFU Sub-Commands](#dfu-sub-commands).

H = Host
C = Chili

## DFU Sub-Commands

The EVBME DFU command includes an element which describes the actual function of the command.

| Sub-Command | ID   | Description
| ----------- | ---- | -----------
| REBOOT      | 0x00 | Reboot the device into the specified mode.
| ERASE       | 0x01 | Erase the specified flash pages
| WRITE       | 0x02 | Write the specified data to flash
| CHECK       | 0x03 | Verify the specified flash range with a checksum
| STATUS      | 0x04 | Status code returned from Chili to host after any other command
| BOOTMODE    | 0x05 | Set the default power-on boot mode of the chili.

