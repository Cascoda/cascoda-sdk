# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.13.1)
project (cascoda-sdk)

if (MSVC)
	message(FATAL_ERROR "Building with the Microsoft Visual C/C++ compiler is not supported. Please set up MinGW as shown here: https://github.com/Cascoda/cascoda-sdk/blob/master/docs/guides/development-setup.md#windows")
endif()

# Include helper functions
include(etc/cascoda.cmake)
include(etc/options.cmake)

include(CTest)

# set the Cascoda src&bin dir
set(CASCODA_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(CASCODA_BINARY_DIR ${PROJECT_BINARY_DIR})

# set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# set the C standards
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

# enable assembly support
enable_language(ASM)

# Include testing library
if(BUILD_TESTING AND (UNIX OR MINGW))
	enable_testing()
	add_subdirectory(third-party/cmocka-1.1.3)
	cascoda_put_subdir(test cmocka)
endif()

# Sub-project configuration ---------------------------------------------------

# API
add_subdirectory(ca821x-api)
add_subdirectory(cascoda-util)

# Posix
if((UNIX OR MINGW) AND (NOT CASCODA_BUILD_DUMMY))
	add_subdirectory(posix)
endif()

# Baremetal
add_subdirectory(baremetal)

# IoTivity-Lite port & source code
add_subdirectory(ocf)

# Openthread
add_subdirectory(openthread)


# Third party
add_subdirectory(third-party EXCLUDE_FROM_ALL)

# Helper targets, only if Cascoda SDK is the root project ---------------------

if(CASCODA_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	# Doxygen
	add_subdirectory(docs)

	# Add clang-format target
	add_custom_target(format
		COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/etc/clang-format.cmake
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)
	
	# Add longtest target
	add_custom_target(longtest
		COMMAND ${CMAKE_COMMAND} -D LONGTEST_CSV_PATH="${CMAKE_BINARY_DIR}/longtests.csv" -P ${PROJECT_SOURCE_DIR}/etc/run-longtests.cmake
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		)
endif()
