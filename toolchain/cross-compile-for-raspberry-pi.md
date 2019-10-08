# How to Cross Compile for the Raspberry Pi #

This repository comes with support for cross-compilation for the Raspberry
Pi. However, before you can use it, you must set up the Raspberry Pi toolchain:

```
# Clone raspi tools to a suitable directory on your machine
git clone https://github.com/raspberrypi/tools --depth=1
# Go to compiler bin dir, add to PATH
cd tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/
export PATH=`pwd`:$PATH
```

Remember exporting the PATH will not persist across sessions, so add this to a suitable startup script if persistance is required.

Afterwards, return to the Cascoda SDK, make a build directory in the normal way and use the arm_gcc_raspberrypi toolchain file:

```
cd ~/cascoda-sdk
mkdir sdk-raspberrypi
cd sdk-raspberrypi
cmake ../cascoda-sdk -DCMAKE_TOOLCHAIN_FILE=toolchain/arm_gcc_raspberrypi.cmake
make -j12
```
