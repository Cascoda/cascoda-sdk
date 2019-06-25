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

# Include testing library
if(BUILD_TESTING AND (UNIX OR MINGW))
	enable_testing()
	add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/cmocka-1.1.3 ${PROJECT_BINARY_DIR}/third-party/cmocka-1.1.3)
endif()

# Sub-project configuration ---------------------------------------------------
macro(cascoda_subdir path)
	add_subdirectory(${PROJECT_SOURCE_DIR}/${path} ${PROJECT_BINARY_DIR}/${path})
endmacro()

# API
cascoda_subdir(ca821x-api)

# Posix
if(UNIX)
	cascoda_subdir(posix/ca821x-posix)
	cascoda_subdir(posix/ca821x-posix-thread)
endif()

# Baremetal platform
cascoda_subdir(baremetal/cascoda-bm-driver)
cascoda_subdir(baremetal/sensorif)
cascoda_subdir(baremetal/platform/cascoda-nuvoton-chili)
cascoda_subdir(baremetal/platform/cascoda-nuvoton-chili2)
cascoda_subdir(baremetal/platform/cascoda-dummy-posix)

# Openthread
cascoda_subdir(openthread)

# Baremetal Applications (If a suitable platform layer was configured)
if(TARGET cascoda-bm-plat)
	cascoda_subdir(baremetal/app/ot-baremetal)
	cascoda_subdir(baremetal/app/tempsense-bm)
	cascoda_subdir(baremetal/app/test15_4-baremetal)
	cascoda_subdir(baremetal/app/sensorif-bm)
endif()

# Add clang-format target
add_custom_target(format
	COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/clang-format.cmake
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)


