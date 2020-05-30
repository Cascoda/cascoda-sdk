set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR M2351)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(COMMON_FLAGS "-mcpu=cortex-m23 -DCORTEX_M23 -march=armv8-m.base -mthumb -mlittle-endian -ffunction-sections -fdata-sections -finline-functions -funsigned-char -Os -g -Wl,--gc-sections")

set(CMAKE_CXX_FLAGS "--specs=nano.specs --specs=nosys.specs ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "--specs=nano.specs --specs=nosys.specs ${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "--specs=nano.specs --specs=nosys.specs ${COMMON_FLAGS}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
