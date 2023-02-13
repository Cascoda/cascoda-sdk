# knx-gen-data

This application generates binary files containing the KNX serial number,
in the format expected by Chilis flashed with KNX-IoT binaries.

Running `knx-gen-data.exe serialno.bin 00fa01abcd ` creates a file called
`serialno.bin` in the current directory.  The command for flashing
`serialno.bin` onto a Chili is `chilictl flash -m serialno.bin`

Once flashed onto a Chili's manufacturer data page, KNX binaries running on
that Chili will take up the serial number "00FA01ABCD".
