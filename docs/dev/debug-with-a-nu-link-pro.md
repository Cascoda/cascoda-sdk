# Nuvoton OpenOCD Guide #

[OpenOCD-Nuvoton.zip](https://github.com/Cascoda/cascoda-sdk/files/10031853/OpenOCD-Nuvoton.zip)

This archive contains all the necessary files for running Nuvoton's OpenOCD
build on 32- and 64-bit Windows machine. Unfortunately, GitHub does not allow
the upload of 7z files, so I have doubly-compressed it as a .zip file to work
around this.

## Note on flashing ##

Unfortunately, flashing through the OpenOCD GDB server is not possible: you
**must not run any GDB `load` commands**! Attempting to do so puts OpenOCD in
an invalid state that is difficult to recover from, and may flash the chip with
corrupted data. If you have used a `load` command, you may need to change the
port that OpenOCD runs on, as detailed below.

Instead, we recommend flashing your binaries using [the Nuvoton ICP Programming
Tool](https://www.nuvoton.com/tool-and-software/software-development-tool/programmer/)
before starting the GDB server.

## How to start the GDB server ##

After you have extracted the contents of the archive, open a shell in the
`src/` directory within the extracted folder. Then, execute the following
command:

```
./openocd.exe -f ../tcl/interface/nulink.cfg -f ../tcl/target/numicroM23.cfg -c "gdb_port 3335"
```

This guide assumes you are using the MinGW Bash shell - the syntax for running
`openocd.exe` may be different for Powershell or the Windows command line.

The first two arguments configure OpenOCD to use the NuLink debugger, and to
debug a Nuvoton M23 CPU, respectively. The final arguments sets the port that
the GDB server will run on. There is nothing special about the number 3335, and
the default port number is 3333 (selected when you delete `-c "gdb_port 3335"`
from the invocation). This argument has been added because ports may fail to be
released by OpenOCD in the event of a crash (such as when attempting to flash a
binary through GDB).

If you have just flashed a binary using the NuMicro ICP Programming Tool, you
must disconnect from the NuLink, or OpenOCD will not be able to use the
hardware.

## How to start the GDB client ##

First, you must start the platform-specific Gnu debugger. We recommend doing
this within the build directory, for easy access to object files.  

```
$ arm-none-eabi-gdb 
```

Then, you must connect to the GDB server:

```
(gdb) target extended-remote localhost:3335
```

The port that the client connects to (port 3335 in this example) must match the
port that the server is running on. This command assumes that the GDB client is
running on the same machine. If the server is running on a different machine,
you must replace `localhost` with the local IP address of that machine.

## OpenOCD specific commands ##

The main command you need to know is `monitor reset halt`. This resets the
microcontroller and halts the CPU within the reset handler. 
- Note that `monitor reset` does not actually halt the microcontroller, which
  is what the Segger GDB server would do. It can then be halted with `monitor
halt`.

You can see the other available commands by using `monitor help`. There is some
overlap in the commands presented by the server (accessed through `monitor`)
and native GDB instructions. We recommend using the corresponding GDB command
if both are available.

You must give GDB access to the debugging symbols corresponding to the flashed
binary in order to use GDB to its full extent. You can do this with the `file`
command. For instance, if you flashed your Chili with `sed-standalone.bin`, you
would run: ``` (gdb) file bin/sed-standalone ```
