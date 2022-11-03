#! /bin/bash

# This script will properly install the latest version of the Arm GNU toolchain.

# Make sure this script is only ever being sourced, and never being executed.
if [[ "$(basename -- "$0")" == "arm_gnu_toolchain_setup.sh" ]];
then
	>&2 echo "Don't run $0, source it."
	>&2 echo "(i.e. type \"source arm_gnu_toolchain_setup.sh\" instead of \"./arm_gnu_toolchain_setup.sh\")"
	exit 1
fi

# Clean up function to be performed upon exit, regardless of success or failure
cleanup () {
	echo -e "\033[33;7mCleaning up...\033[0m"
	
# Delete source and executable used for checking that compilation works 
	rm main.c test

# Remove the archive file downloaded
	rm -rf gcc-arm-none-eabi.tar.bz2
}



# Uninstalls, if it exists, the current version of the toolchain which was previously installed using a package manager. 
echo -e "\033[33;7mUninstalling previous installation done via package manager...\033[0m"
sudo apt-get remove gcc-arm-none-eabi
sudo apt-get autoremove

# Grab the latest version number from the official website.
echo -e "\033[33;7mInstalling latest version of Arm GNU Toolchain from official website...\033[0m"
ARM_TOOLCHAIN_VERSION=$(curl -s https://developer.arm.com/downloads/-/gnu-rm | grep -Po '<h3>Version \K.+(?= <span)')

# Download the archive file matching that version.
curl -Lo gcc-arm-none-eabi.tar.bz2 "https://developer.arm.com/-/media/Files/downloads/gnu-rm/${ARM_TOOLCHAIN_VERSION}/gcc-arm-none-eabi-${ARM_TOOLCHAIN_VERSION}-x86_64-linux.tar.bz2"

# Create a new directory to store the toolchain files.
sudo rm -rf /opt/gcc-arm-none-eabi
sudo mkdir /opt/gcc-arm-none-eabi

# Extract the toolchain files to the specified directory.
sudo tar xf gcc-arm-none-eabi.tar.bz2 --strip-components=1 -C /opt/gcc-arm-none-eabi

# Add /opt/gcc-arm-none-eabi/bin directory to the PATH environment variable.
sudo rm -rf /etc/profile.d/gcc-arm-none-eabi.sh
echo 'export PATH=/opt/gcc-arm-none-eabi/bin:$PATH' | sudo tee -a /etc/profile.d/gcc-arm-none-eabi.sh

# Allow this change to take effect.
source /etc/profile

# Verify installation
echo -e "\033[33;7mVerifying installation...\033[0m"
if arm-none-eabi-gcc --version | grep $ARM_TOOLCHAIN_VERSION
then
        echo -e "\033[33;5;7mINSTALLATION SUCCESSFUL\033[0m"
else
        echo -e "\033[33;5;7mINSTALLATION FAILED\033[0m"
	cleanup
	exit 1
fi	

# Install libncurses, which is a dependency necessary for arm-none-eabi-gdb.
echo -e "\033[33;7mInstalling libncurses5 dependency for arm-none-eabi-gdb...\033[0m"
sudo apt-get install libncurses5

# Check that arm-none-eabi-gdb has the required dependencies
echo -e "\033[33;7mVerifying that arm-none-eabi-gdb has the required dependencies...\033[0m"
if arm-none-eabi-gdb --version | grep $ARM_TOOLCHAIN_VERSION
then
        echo -e "\033[33;5;7mDEPENDENCIES SUCCESSFULLY INSTALLED FOR arm-none-eabi-gdb\033[0m"
else
        echo -e "\033[33;5;7mMISSING DEPENDENCIES FOR arm-none-eabi-gdb\033[0m"
	cleanup
	exit 1
fi	


# Check installation
# Create simple program to compile
echo -e "\033[33;7mChecking that a C program can be compiled using the installed compiler...\033[0m"
echo $'#include <stdio.h>\n\nint main()\n{\n\tprintf("Hello, world!\\n");\n\treturn 0;\n}' > main.c
cat main.c

# Compile the program
arm-none-eabi-gcc --specs=rdimon.specs main.c -o test

# Use the file command to verify that the executable generated is for ARM architecture
if file test | grep "ARM"
then
        echo -e "\033[33;5;7mCOMPILATION ATTEMPT SUCCESSFUL\033[0m"
else
        echo -e "\033[33;5;7mCOMPILATION ATTEMPT FAILED\033[0m"
	cleanup
	exit
fi

cleanup
