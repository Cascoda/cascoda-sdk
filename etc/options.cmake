# Cmake file containing all of the high-level configure options and parameters for the cascoda build
# This file is processed before anything else, allowing options that affect the whole build. Options
# only affect certain modules should be kept in their respective projects.

option(CASCODA_BUILD_OCF "Whether to build Iotivity-Lite. Turning this on will also force OT_BUILTIN_MBEDTLS to be off. Currently only supported on Chili 2." OFF)

option(CASCODA_BUILD_SECURE_LWM2M "Whether to build LWM2M stack wakaama with security. Incompatible with CASCODA_BUILD_OCF." OFF)

option(CASCODA_BUILD_LWIP "Build the LwIP stack into the Cascoda SDK" ON)

option(CASCODA_BUILD_DUMMY "Build the dummy baremetal layer for posix instead of the posix SDK. This replaces standard posix behaviour. Only for unit testing baremetal." OFF)
mark_as_advanced(FORCE CASCODA_BUILD_DUMMY)

if(CASCODA_BUILD_OCF AND CASCODA_BUILD_SECURE_LWM2M)
	message(FATAL_ERROR "Not possible to enable CASCODA_BUILD_OCF and CASCODA_BUILD_SECURE_LWM2M simultaneously")
endif()
