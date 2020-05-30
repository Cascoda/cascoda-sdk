# Global config ---------------------------------------------------------------
project (cascoda-nuvoton-chili2)

# Return if not suitable for system processor
if(NOT (CMAKE_SYSTEM_PROCESSOR STREQUAL "M2351"))
	return()
endif()

# Return if platform already configured
if(TARGET cascoda-bm-plat)
	return()
endif()

# Set configuration variables -------------------------------------------------
cascoda_dropdown(CASCODA_CHILI2_REV
	"The revision of the chili 2 hardware being used 2.x (2.0 is chili, use -1 for numaker devboard)"
	0 -1
	)

# Single or double sided config
cascoda_dropdown(CASCODA_CHILI2_CONFIG_STRING
	"The hardware configuration of the Chili 2"
	TWO_SIDED ONE_SIDED
	)
cascoda_map(${CASCODA_CHILI2_CONFIG_STRING} CASCODA_CHILI2_CONFIG
	ONE_SIDED 0
	TWO_SIDED 1
	)

# Enable trustzone
set(CASCODA_CHILI2_TRUSTZONE OFF CACHE BOOL "Enable the Chili2 armv8-m CSME build (TrustZone)")

# Number of flash pages
set(CASCODA_CHILI_FLASH_PAGES 2 CACHE STRING "The number of 2048 byte flash pages that should be used for user flash")

cascoda_mark_important("CASCODA_CHILI_FLASH_PAGES")
# UART Baud config
if(CASCODA_BM_INTERFACE STREQUAL "UART")
	cascoda_dropdown(CASCODA_CHILI2_UART_CHANNEL
		"The hardware UART channel to use for UART comms"
		0 1 2 4 5
		)
	set(CASCODA_CHILI2_UART_BAUDRATE 1000000 CACHE STRING "The baud rate to use for UART. Max 6MHz")
	if(CASCODA_CHILI2_UART_BAUDRATE GREATER 6000000)
		message( FATAL_ERROR "Baud rate must be under 6MHz")
	endif()
	if(CASCODA_CHILI2_TRUSTZONE)
		message(FATAL_ERROR "Chili2 trustzone build does not currently support UART, please change either CASCODA_BM_INTERFACE or CASCODA_CHILI2_TRUSTZONE")
	endif()
endif()

# Default system clock
cascoda_dropdown(CASCODA_CHILI2_DEFAULT_FSYS
	"The default system frequency. Minimum is FSYS_12MHZ for USB comms or UART comms > 115200."
	FSYS_48MHZ FSYS_4MHZ FSYS_12MHZ FSYS_16MHZ FSYS_24MHZ FSYS_32MHZ FSYS_64MHZ
	)
if(CASCODA_CHILI2_DEFAULT_FSYS STREQUAL "FSYS_4MHZ")
	if(((CASCODA_BM_INTERFACE STREQUAL "UART") AND CASCODA_CHILI2_UART_BAUDRATE GREATER 115200) OR (CASCODA_BM_INTERFACE STREQUAL "USB"))
		message( FATAL_ERROR "Minimum system clock is FSYS_12MHZ for USB comms or UART comms > 115200 baud.")
	endif()
endif()

# Mark the device as being FTD-capable
set(CASCODA_OPENTHREAD_FTD ON CACHE INTERNAL "")

# Set the message buffers pretty high as we have plenty of RAM on chili2
set(CASCODA_OPENTHREAD_MESSAGE_BUFS 100 CACHE STRING "Integer number of message buffers for openthread to use. Recommend >44")

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/port/include/cascoda_chili_config.h.in"
	"${PROJECT_BINARY_DIR}/port/include/cascoda_chili_config.h"
	)

# Sub-project configuration ---------------------------------------------------

add_subdirectory(vendor)
add_subdirectory(third-party/freertos)
add_subdirectory(third-party/mbedtls)
add_subdirectory(chili2-default-secure)

# Configure partition.h accordingly for vendorlibs
target_include_directories(nuvoton-vendor-m2351 PUBLIC ${PROJECT_SOURCE_DIR}/port/include_notz)
target_include_directories(nuvoton-vendor-m2351-secure PUBLIC ${PROJECT_SOURCE_DIR}/port/include_tz)
target_include_directories(nuvoton-vendor-m2351-nonsecure PUBLIC ${PROJECT_SOURCE_DIR}/port/include_tz)

# Main library config ---------------------------------------------------------
set(CHILI2_SOURCES
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_gpio_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_uart_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_sensorif_m2351.c
	${PROJECT_SOURCE_DIR}/third-party/mbedtls/TARGET_M2351/aes_alt.c
	${PROJECT_SOURCE_DIR}/third-party/mbedtls/TARGET_M2351/hardware_poll.c
	${PROJECT_SOURCE_DIR}/third-party/mbedtls/TARGET_M2351/ecp_internal_alt.c
	${PROJECT_SOURCE_DIR}/third-party/mbedtls/TARGET_M2351/crypto-misc.cpp
	)

set(CHILI2_SECURE_SOURCES
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_chili_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_dataflash_m2351_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_rtc_m2351_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_sensorif_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_time_secure.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_m2351_secure.c
)

# Add linker file dependency
add_library(cascoda-chili2-ldfile INTERFACE)
add_library(cascoda-chili2-ldfile-ldrom INTERFACE)

if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
	target_link_options(cascoda-chili2-ldfile
		INTERFACE
			"-T${PROJECT_SOURCE_DIR}/m2351BN.ld"
		)
	target_link_options(cascoda-chili2-ldfile-ldrom
		INTERFACE
			"-T${PROJECT_SOURCE_DIR}/m2351BN_LDROM.ld"
		)
elseif(CMAKE_C_COMPILER MATCHES armclang)
	target_link_options(cascoda-chili2-ldfile
		INTERFACE
			"-Wl,--ro-base=0x00000000,--entry=0x00000000,--rw-base=0x20000000,--entry=Reset_Handler,--first=__Vectors,--strict,--datacompressor=off,--entry=Reset_Handler,--library_type=microlib"
		)
	target_link_options(cascoda-chili2-ldfile-ldrom
		INTERFACE
			"-Wl,--ro-base=0x00100000,--entry=0x00100000,--rw-base=0x20000000,--entry=Reset_Handler,--first=__Vectors,--strict,--datacompressor=off,--entry=Reset_Handler,--library_type=microlib"
		)
endif()

if(NOT CASCODA_CHILI2_TRUSTZONE)
	# Non-TrustZone Build ----------------------------------------------------------------------------------------------------------------
	add_library(cascoda-chili2
		${CHILI2_SOURCES}
		${CHILI2_SECURE_SOURCES}
		$<TARGET_OBJECTS:nuvoton-vendor-m2351>
	)

	#The non-trustzone chili2 platform can use the default partition file, so explicitly add this compile define here to dodge the error.
	target_compile_definitions(cascoda-chili2 PRIVATE CUSTOM_PARTITION_H)

	target_link_libraries(cascoda-chili2
		PUBLIC
			cascoda-bm
			nuvoton-vendor-m2351
			cascoda-chili2-ldfile
		PRIVATE
			mbedcrypto
			mbedcrypto-plat
		)

	cascoda_use_warnings(cascoda-chili2)

	target_include_directories( cascoda-chili2
		PRIVATE
			${PROJECT_SOURCE_DIR}/third-party/mbedtls/platform
		PUBLIC
			${PROJECT_SOURCE_DIR}/port/include
			${PROJECT_BINARY_DIR}/port/include
		)

	# Export the chili2 platform as cascoda-bm-plat
	add_library(cascoda-bm-plat ALIAS cascoda-chili2)

else()
	# Use TrustZone Build ----------------------------------------------------------------------------------------------------------------
	add_library(cascoda-chili2-secure OBJECT
		${CHILI2_SECURE_SOURCES}
	)

	target_link_libraries(cascoda-chili2-secure
		PUBLIC
			nuvoton-vendor-m2351-secure
		)

	cascoda_tz_build(cascoda-chili2-secure)

	target_include_directories(cascoda-chili2-secure
		PUBLIC
			${PROJECT_SOURCE_DIR}/port/include_tz
			${PROJECT_SOURCE_DIR}/port/include
			${PROJECT_BINARY_DIR}/port/include
			$<TARGET_PROPERTY:cascoda-bm,INTERFACE_INCLUDE_DIRECTORIES>
			$<TARGET_PROPERTY:ca821x-api,INTERFACE_INCLUDE_DIRECTORIES>
		)

	add_library(cascoda-chili2-nonsecure
		${CHILI2_SOURCES}
	)

	target_include_directories(cascoda-chili2-nonsecure
		PRIVATE
			${PROJECT_SOURCE_DIR}/third-party/mbedtls/platform
		PUBLIC
			${PROJECT_SOURCE_DIR}/port/include_tz
			${PROJECT_SOURCE_DIR}/port/include
			${PROJECT_BINARY_DIR}/port/include
		)

	target_link_libraries(cascoda-chili2-nonsecure
		PUBLIC
			cascoda-bm
			nuvoton-vendor-m2351-nonsecure
			chili2-default-secure-implib
		PRIVATE
			mbedcrypto
			mbedcrypto-plat
		)

	# Export the chili2 ns platform as cascoda-bm-plat
	add_library(cascoda-bm-plat ALIAS cascoda-chili2-nonsecure)
endif()

# Variables for configuring memory usage using the cascoda_configure_memory CMake function
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set(CASCODA_STARTUP_ASM ${CASCODA_SOURCE_DIR}/baremetal/platform/cascoda-nuvoton-chili2/vendor/M2351Library/Device/Nuvoton/M2351/Source/GCC/startup_M2351.S CACHE INTERNAL "")
	set(CASCODA_STACK_SIZE Stack_Size CACHE INTERNAL "")
	set(CASCODA_HEAP_SIZE Heap_Size CACHE INTERNAL "")
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CASCODA_STARTUP_ASM ${CASCODA_SOURCE_DIR}/baremetal/platform/cascoda-nuvoton-chili2/vendor/M2351Library/Device/Nuvoton/M2351/Source/GCC/startup_M2351.s CACHE INTERNAL "")
	set(CASCODA_STACK_SIZE Stack_Size CACHE INTERNAL "")
	set(CASCODA_HEAP_SIZE Heap_Size CACHE INTERNAL "")
endif()

if(CASCODA_BM_USE_USB)
	add_executable(ldrom_hid
		${PROJECT_SOURCE_DIR}/LDROM/ldrom_hid.c
		${PROJECT_SOURCE_DIR}/LDROM/hid_transfer.c
	)
	target_link_libraries(ldrom_hid cascoda-chili2 cascoda-chili2-ldfile-ldrom)
	cascoda_make_binary(ldrom_hid)
endif()
