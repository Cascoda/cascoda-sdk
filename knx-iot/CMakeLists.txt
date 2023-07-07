project(knx-iot)
cmake_minimum_required(VERSION 3.13)

if(NOT CASCODA_BUILD_KNX)
    return()
endif()

add_compile_definitions(OC_DYNAMIC_ALLOCATION)
if(WIN32 OR UNIX)
    set(KNX_BUILTIN_MBEDTLS ON CACHE BOOL "" FORCE)
else()
set(KNX_BUILTIN_MBEDTLS OFF CACHE BOOL "" FORCE)
endif()
set(KNX_BUILTIN_TINYCBOR OFF CACHE BOOL "" FORCE)
set(OT_SRP_CLIENT ON CACHE BOOL "" FORCE)
set(OC_ECDSA ON CACHE BOOL "" FORCE)

if(WIN32 OR UNIX)
    set(KNX_SPAKE_MIN_IT "10000" CACHE STRING "Minimum number of SHA256 iterations used within the SPAKE2+ handshake")
    set(KNX_SPAKE_MAX_IT "100000" CACHE STRING "Maximum number of SHA256 iterations used within the SPAKE2+ handshake")
else()
    # Since we do not have hardware acceleration of SHA256, we reduce the number of iterations to enable proof-of-concept
    # testing of Spake2+. This makes the handshake significantly less secure - the ideal solution for production is to
    # pre-compute the result of the hash & use the hardcoded result during the handshake.
    set(KNX_SPAKE_MIN_IT "100" CACHE STRING "Minimum number of SHA256 iterations used within the SPAKE2+ handshake")
    set(KNX_SPAKE_MAX_IT "200" CACHE STRING "Maximum number of SHA256 iterations used within the SPAKE2+ handshake")
endif()

set(KNX_LOG_CAST_64_BIT_INTS_ENABLED ON CACHE BOOL "Cast int64_t and uint64_t values to (int) if platform does not supports 64 bit int format specifiers")

include(FetchContent)

FetchContent_Declare(
    knx-iot-stack
    GIT_REPOSITORY https://github.com/KNX-IOT/KNX-IOT-STACK.git
    GIT_TAG master
)

if(NOT knx-iot-stack_POPULATED)
	FetchContent_Populate(knx-iot-stack)
	add_subdirectory(${knx-iot-stack_SOURCE_DIR} ${knx-iot-stack_BINARY_DIR})
endif()

#Don't build any of the port or demo apps on POSIX
if(UNIX OR WIN32)
    return()
endif()

add_subdirectory(port)
