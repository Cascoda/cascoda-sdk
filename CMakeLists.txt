# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.13.1)
project (cascoda-sdk)

# Include helper functions
include(cascoda.cmake)
include(CTest)

# set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set the C standards
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

# enable assembly support
enable_language(ASM)

# Include testing library
if(BUILD_TESTING AND (UNIX OR MINGW))
	enable_testing()
	add_subdirectory(third-party/cmocka-1.1.3)
endif()

# Sub-project configuration ---------------------------------------------------

# API
add_subdirectory(ca821x-api)

# Posix
if(UNIX OR MINGW)
	add_subdirectory(posix/ca821x-posix)
endif()

if(UNIX)
	add_subdirectory(posix/ca821x-posix-thread)
endif()

# Baremetal
add_subdirectory(baremetal)

# Openthread
add_subdirectory(openthread)

# UZLib, used for decompressing gzipped data
add_subdirectory(third-party/uzlib EXCLUDE_FROM_ALL)
# qrcodegen, used for building QR codes
add_subdirectory(third-party/QR-Code-generator EXCLUDE_FROM_ALL)
# hidapi, needed by the Windows build of ca821x-posix
add_subdirectory(third-party/hidapi EXCLUDE_FROM_ALL)
# tinycbor-master, used for CBOR encoding and decoding
add_subdirectory(third-party/tinycbor-master EXCLUDE_FROM_ALL)

# Doxygen
add_subdirectory(docs)

# Add clang-format target
add_custom_target(format
	COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/clang-format.cmake
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
