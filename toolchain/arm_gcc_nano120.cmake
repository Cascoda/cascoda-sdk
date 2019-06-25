set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR NANO120)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(COMMON_FLAGS "-mcpu=cortex-m0 -DCORTEX_M0 -mthumb -ffunction-sections -fdata-sections -finline-functions -funsigned-char -funsigned-bitfields -g -Os")

set(CMAKE_CXX_FLAGS "${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${COMMON_FLAGS} --specs=nano.specs --specs=nosys.specs -lc -Wl,--gc-sections" CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
