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

if(CASCODA_CHILI2_TRUSTZONE AND CASCODA_EXTERNAL_FLASHCHIP_PRESENT)
	message(FATAL_ERROR "Not possible to enable CASCODA_CHILI2_TRUSTZONE and CASCODA_EXTERNAL_FLASHCHIP_PRESENT simultaneously")
endif()

cascoda_has_changed(CASCODA_BUILD_OCF ocf_has_changed)

if(ocf_has_changed AND CASCODA_BUILD_OCF)
    set(ot_config_value OFF)
    set(bufs_value 85)
    set(flash_pages 16)

    set(OT_BORDER_AGENT ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_BORDER_ROUTER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_COAP ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_COMMISSIONER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_DHCP6_SERVER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_MAC_FILTER ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_SERVICE ${ot_config_value} CACHE BOOL "" FORCE)
    set(OT_UDP_FORWARD ${ot_config_value} CACHE BOOL "" FORCE)

    set(CASCODA_OPENTHREAD_MESSAGE_BUFS ${bufs_value} CACHE STRING "" FORCE)
    set(CASCODA_CHILI_FLASH_PAGES ${flash_pages} CACHE STRING "" FORCE)
endif()