# Cascoda Development Board

This is built for cascoda development board only with Chili2S. It has examples for controlling LEDs and sensors on board.

To correctly build for the chili development board:
```CMake
# Edit CMakeCache.txt in sdk-chili2 directory
CASCODA_CHILI2_CONFIG_STRING:STRING=DEV_BOARD
CASCODA_CHILI2_REV:STRING=1
```