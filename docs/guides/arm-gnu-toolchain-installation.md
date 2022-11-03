# Installation of Arm GNU Toolchain on Linux

The [GNU Arm Embedded Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) is a collection of packages such as GCC, GDB, etc. used for embedded systems software development. Follow the steps in this guide in order to properly install the latest version of the toolchain.

## Automatic Installation

If you wish to automatically install the Arm GNU Toolchain for Linux, source the script called [arm_gnu_toolchain_setup.sh](/etc/arm_gnu_toolchain_setup.sh).
Note: Sourcing a script (in contrast to executing a script) means running it in the following manner:

```bash
source /etc/arm_gnu_toolchain_setup.sh
```
If you wish to manually install the toolchain instead, follow the steps outlined below.

## Step 1 - Uninstall existing version

It is important to uninstall whatever existing version of the toolchain that might have been installed on your system using your linux distribution's package manager. Installations not done directly from the Arm developer website can result in various issues down the line. To do that, run the following command:

```bash
sudo apt-get remove gcc-arm-none-eabi
```

Check that this worked by typing the following command:

```bash
arm-none-eabi-gcc --version
```

This should result in something that looks like the following (exact output differs slightly between linux distributions):

```bash
-bash: arm-none-eabi-gcc: command not found
```


## Step 2 - Install the toolchain manually from the official website

Type the following command to grab the latest version number from the official website.

```bash
ARM_TOOLCHAIN_VERSION=$(curl -s https://developer.arm.com/downloads/-/gnu-rm | grep -Po '<h3>Version \K.+(?= <span)')
```

Then type the following to download the archive file of that version.

```bash
curl -Lo gcc-arm-none-eabi.tar.bz2 "https://developer.arm.com/-/media/Files/downloads/gnu-rm/${ARM_TOOLCHAIN_VERSION}/gcc-arm-none-eabi-${ARM_TOOLCHAIN_VERSION}-x86_64-linux.tar.bz2"
```

Create a new directory to store the toolchain files.

```bash
sudo mkdir /opt/gcc-arm-none-eabi
```

Extract the toolchain files to the specified directory.

```bash
sudo tar xf gcc-arm-none-eabi.tar.bz2 --strip-components=1 -C /opt/gcc-arm-none-eabi
```

Add `/opt/gcc-arm-none-eabi/bin` directory to the `PATH` environment variable.

```bash
echo 'export PATH=/opt/gcc-arm-none-eabi/bin:$PATH' | sudo tee -a /etc/profile.d/gcc-arm-none-eabi.sh
```

For those changes to take effect, run the following command.

```bash
source /etc/profile
```

Check that the installation was successful by checking the version of the compilers.

```bash
user@hostname:~$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (GNU Arm Embedded Toolchain 10.3-2021.10) 10.3.1 20210824 (release)
Copyright (C) 2020 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

user@hostname:~$ arm-none-eabi-g++ --version
arm-none-eabi-g++ (GNU Arm Embedded Toolchain 10.3-2021.10) 10.3.1 20210824 (release)
Copyright (C) 2020 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

Install `libncurses5`, which is a dependency necessary for `arm-none-eabi-gdb`, which is a debugger.

```bash
sudo apt-get install libncurses5
```

Check that this worked by checking the version of the debugger.

```bash
user@hostname:~$ arm-none-eabi-gdb --version
GNU gdb (GNU Arm Embedded Toolchain 10.3-2021.10) 10.2.90.20210621-git
Copyright (C) 2021 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
```

Remove the now unnecessary archive file (or keep it if you prefer).

```bash
rm -rf gcc-arm-none-eabi.tar.bz2
```

## Step 3 - Testing the toolchain 

Create a `main.c` file:

```bash
nano main.c
```

Add the following code:

```c
#include <stdio.h>

int main()
{
    printf("Hello world\n");

    return 0;
}
```

Compile the program.

```bash
arm-none-eabi-gcc --specs=rdimon.specs main.c -o test
```

Now use the `file` command to verify that the executable generated is for ARM architecture.

```bash
user@hostname:~$ file test
test: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), statically linked, not stripped
```

## Extra: How to uninstall the toolchain

If you want to completely remove the GNU Arm Embedded toolchain, delete the installation diretory.

```bash
sudo rm -rf /opt/gcc-arm-none-eabi
```

Remove `gcc-arm-none-eabi.sh` file that is used to set the environment variable.

```bash
sudo rm -rf /etc/profile.d/gcc-arm-none-eabi.sh
```