# knx-gen-data

This application generates binary files containing the KNX serial number,
in the format expected by Chilis flashed with KNX-IoT binaries.

Running `knx-gen-data.exe serialno.bin 00fa01abcd ` creates a file called
`serialno.bin` in the current directory.  
This also generates a random device password and precomputed SPAKE2+ records 
to allow much higher PBKDF iteration counts and improve security of OC_SPAKE.
The password and other parameters of knx-gen-data can also be manually 
controlled as additional command line arguments. The form for this is 
`knx-gen-data <output-file.bin> <serial-number> [password [eui64 [thread_pw [salt]]]]`.  
The command for flashing `serialno.bin` onto a Chili is 
`chilictl flash -m serialno.bin`

Once flashed onto a Chili's manufacturer data page, KNX binaries running on
that Chili will take up the serial number "00FA01ABCD" and the generated pasword. The QR code data is printed as output by knx-gen-data.
