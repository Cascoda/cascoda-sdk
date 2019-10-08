# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
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

# Baremetal platform
add_subdirectory(baremetal/cascoda-bm-core)
add_subdirectory(baremetal/cascoda-bm-driver)
add_subdirectory(baremetal/sensorif)
add_subdirectory(baremetal/platform/cascoda-nuvoton-chili)
add_subdirectory(baremetal/platform/cascoda-nuvoton-chili2)
add_subdirectory(baremetal/platform/cascoda-dummy-posix)

# Openthread
add_subdirectory(openthread)

# UZLib, used for decompressing gzipped data
add_subdirectory(third-party/uzlib)
# qrcodegen, used for building QR codes
add_subdirectory(third-party/QR-Code-generator)
# hidapi, needed by the Windows build of ca821x-posix
add_subdirectory(third-party/hidapi)

# Baremetal Applications (If a suitable platform layer was configured)
if(TARGET cascoda-bm-plat)
	add_subdirectory(baremetal/app/ot-baremetal)
	add_subdirectory(baremetal/app/tempsense-bm)
	add_subdirectory(baremetal/app/test15_4-baremetal)
	add_subdirectory(baremetal/app/sensorif-bm)
	add_subdirectory(baremetal/app/eink-bm)
	add_subdirectory(baremetal/app/ot-sed-sensorif)
	add_subdirectory(baremetal/app/ot-sed-standalone)
	add_subdirectory(baremetal/app/ot-sed-freertos)
	add_subdirectory(baremetal/app/ot-sed-eink)
endif()

# Doxygen
add_subdirectory(docs)

# Add clang-format target
add_custom_target(format
	COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/clang-format.cmake
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
