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

set(KNX_REPO knx-iot-stack)
set(KNX_REPO_URL https://github.com/KNX-IOT/KNX-IOT-STACK.git)

if(DEFINED CASCODA_USE_KNX_REF)
  set(KNX_IOT_STACK_REF ${CASCODA_USE_KNX_REF} CACHE STRING "")
else()
  set(KNX_IOT_STACK_REF master CACHE STRING "")
endif()

if(DEFINED CASCODA_USE_KNX_REPO_URL)
  set(KNX_IOT_STACK_REPO ${CASCODA_USE_KNX_REPO_URL} CACHE STRING "")
else()
  set(KNX_IOT_STACK_REPO ${KNX_REPO_URL} CACHE STRING "")
endif()

FetchContent_Declare(
    ${KNX_REPO}
    GIT_REPOSITORY ${KNX_IOT_STACK_REPO}
    GIT_TAG ${KNX_IOT_STACK_REF}
)

if(NOT ${KNX_REPO}_POPULATED)
	FetchContent_Populate(${KNX_REPO})
	add_subdirectory(${${KNX_REPO}_SOURCE_DIR} ${${KNX_REPO}_BINARY_DIR})
endif()

#Don't build any of the port or demo apps on POSIX
if(UNIX OR WIN32)
    return()
endif()

add_subdirectory(port)
