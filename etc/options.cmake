# Cmake file containing all of the high-level configure options and parameters for the cascoda build
# This file is processed before anything else, allowing options that affect the whole build. Options
# only affect certain modules should be kept in their respective projects.

option(CASCODA_BUILD_OCF "Whether to build Iotivity-Lite. Turning this on will also force OT_BUILTIN_MBEDTLS to be off. Currently only supported on Chili 2." OFF)

option(CASCODA_BUILD_KNX "Whether to build the KNX-IoT stack.")

option(CASCODA_BUILD_OT "Whether to build OpenThread" ON)
mark_as_advanced(CASCODA_BUILD_OT)

option(CASCODA_BUILD_SECURE_LWM2M "Whether to build LWM2M stack wakaama with security. Incompatible with CASCODA_BUILD_OCF." OFF)
mark_as_advanced(CASCODA_BUILD_SECURE_LWM2M)

option(CASCODA_BUILD_LWIP "Build the LwIP stack into the Cascoda SDK" OFF)
mark_as_advanced(CASCODA_BUILD_LWIP)

option(CASCODA_BUILD_DUMMY "Build the dummy baremetal layer for posix instead of the posix SDK. This replaces standard posix behaviour. Only for unit testing baremetal." OFF)
mark_as_advanced(FORCE CASCODA_BUILD_DUMMY)

option(CASCODA_BUILD_BINARIES "Whether to build the Cascoda example applications by default. Set this to OFF if you only need the libraries - the binaries will be excluded from the default 'ALL' target" ON)
mark_as_advanced(CASCODA_BUILD_BINARIES)

if(CASCODA_BUILD_OCF AND CASCODA_BUILD_SECURE_LWM2M)
	message(FATAL_ERROR "Not possible to enable CASCODA_BUILD_OCF and CASCODA_BUILD_SECURE_LWM2M simultaneously")
endif()

if(CASCODA_BUILD_OCF AND CASCODA_BUILD_KNX)
	message(FATAL_ERROR "Not possible to enable CASCODA_BUILD_OCF and CASCODA_BUILD_KNX simultaneously")
endif()

if(CASCODA_BUILD_OT AND CASCODA_BUILD_KNX AND (WIN32 OR UNIX))
    message(FATAL_ERROR "Not possible to build KIS AND OpenThread for posix due to MbedTLS clash")
endif()

cascoda_has_changed(CASCODA_BUILD_OCF ocf_has_changed)

option(CASCODA_PRODUCTION "Whether or not to build the SDK in the production configuration" OFF)

if(CASCODA_PRODUCTION)
	# Cascoda SDK Variables
    set(CASCODA_HF_REBOOT ON CACHE BOOL "" FORCE)

    # This line that sets the log level to CRIT has been commented out.
    # There are two reasons for this:
    # 1. In the above line, we are setting the CASCOCA_BM_INTERFACE to NONE,
    #    whichs means that there wouldn't be any logs in the first place
    # 2. For some reason, having the CASCODA_LOG_LEVEL set to CRIT, and also
    #    having LTO enabled, causes the environment sensor to crash and reboot
    #    upon trying to download the configuration from ETS!!
    #set(CASCODA_LOG_LEVEL CRIT CACHE STRING "" FORCE)

    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "" FORCE)
    # KNX IoT Stack Variables
    set(OC_DEBUG_ENABLED OFF CACHE BOOL "" FORCE)
    set(OC_DEBUG_OSCORE_ENABLED OFF CACHE BOOL "" FORCE)
    set(OC_PRINT_ENABLED OFF CACHE BOOL "" FORCE)
    set(OC_PRINT_APP_ENABLED OFF CACHE BOOL "" FORCE)
    # KNX IoT Code Gen Variables
    set(CASCODA_DO_LTO ON CACHE BOOL "")
endif()

if(ocf_has_changed AND CASCODA_BUILD_OCF)
    set(ot_config_value OFF)
    set(flash_pages 16)

    set(OT_BORDER_AGENT ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_BORDER_ROUTER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_COAP ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_COMMISSIONER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_DHCP6_SERVER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_MAC_FILTER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_SERVICE ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_UDP_FORWARD ${ot_config_value} CACHE BOOL "" FORCE)

    set(CASCODA_CHILI_FLASH_PAGES ${flash_pages} CACHE STRING "" FORCE)
endif()
