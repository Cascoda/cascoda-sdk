set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR M2351)

set(CMAKE_C_COMPILER armclang)
set(CMAKE_ASM_COMPILER armasm)
set(CMAKE_CXX_COMPILER armclang)
set(CASCODA_FROMELF fromelf)

# cmake bug https://cmake.org/pipermail/cmake/2018-February/067021.html
unset(WIN32)

#-Wno-packed -Wno-missing-variable-declarations -Wno-missing-prototypes -Wno-missing-noreturn -Wno-sign-conversion -Wno-nonportable-include-path -Wno-reserved-id-macro -Wno-unused-macros -Wno-documentation-unknown-command -Wno-documentation -Wno-license-management -Wno-parentheses-equality
set(COMMON_FLAGS "--target=arm-arm-none-eabi -mcpu=cortex-m23 -fno-rtti -funsigned-char -D__MICROLIB -mlittle-endian -gdwarf-3 -O1 -ffunction-sections -MD")

set(CMAKE_CXX_FLAGS "${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "--cpu=Cortex-M23 --li -g --pd \"__MICROLIB SETA 1\"")
set(CMAKE_EXE_LINKER_FLAGS "${COMMON_FLAGS} -Wl,--cpu=Cortex-M23" CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
