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

# define the 'interface' library that makes up the bm platform.
add_library(cascoda-bm-plat INTERFACE)

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

# Number of flash pages
set(CASCODA_CHILI_FLASH_PAGES 2 CACHE STRING "The number of 2048 byte flash pages that should be used for user flash")

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
add_subdirectory(freertos-default-secure)
add_subdirectory(examples/freertos-demo-secure)
add_subdirectory(examples/freertos-demo)

# Main library config ---------------------------------------------------------
set(CHILI2_SOURCES
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_gpio_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_uart_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_sensorif_m2351.c
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

add_library(cascoda-chili2
	${CHILI2_SOURCES}
	${CHILI2_SECURE_SOURCES}
	$<TARGET_OBJECTS:nuvoton-vendor-m2351>
)

target_link_libraries(cascoda-chili2
	PUBLIC
		cascoda-bm
		nuvoton-vendor-m2351
	)

cascoda_use_warnings(cascoda-chili2)

target_include_directories( cascoda-chili2
	PUBLIC
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
	)

# Add linker file dependency
add_library(cascoda-chili2-ldfile INTERFACE)

if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
	target_link_options(cascoda-chili2-ldfile
		INTERFACE
			"-T${PROJECT_SOURCE_DIR}/m2351BN.ld"
		)
elseif(CMAKE_C_COMPILER MATCHES armclang)
	target_link_options(cascoda-chili2-ldfile
		INTERFACE
			"-Wl,--ro-base=0x00000000,--entry=0x00000000,--rw-base=0x20000000,--entry=Reset_Handler,--first=__Vectors,--strict,--datacompressor=off,--entry=Reset_Handler,--library_type=microlib"
		)
endif()

# Export the chili2 platform as cascoda-bm-plat
# A target that links to cascoda-bm-plat expects everything to work automagically, so we include
# the linker script here. You can link manually to cascoda-chili2 and to cascoda-chili2-ldfile
# if you want to opt in into just some of the behaviour
target_link_libraries(cascoda-bm-plat INTERFACE cascoda-chili2 cascoda-chili2-ldfile)

add_library(cascoda-chili2-secure INTERFACE)

target_sources(cascoda-chili2-secure INTERFACE
	${CHILI2_SECURE_SOURCES}
)

target_link_libraries(cascoda-chili2-secure
	INTERFACE
		nuvoton-vendor-m2351-secure
	)

target_compile_options(cascoda-chili2-secure INTERFACE -mcmse)
target_link_options(cascoda-chili2-secure INTERFACE -mcmse)

target_include_directories( cascoda-chili2-secure
	INTERFACE
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
		$<TARGET_PROPERTY:cascoda-bm,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:ca821x-api,INTERFACE_INCLUDE_DIRECTORIES>
	)

add_library(cascoda-bm-plat-nonsecure INTERFACE)

target_sources(cascoda-bm-plat-nonsecure INTERFACE
	${CHILI2_SOURCES}
)

target_include_directories(cascoda-bm-plat-nonsecure
	INTERFACE
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
		$<TARGET_PROPERTY:cascoda-bm,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:ca821x-api,INTERFACE_INCLUDE_DIRECTORIES>
	)

target_link_libraries(cascoda-bm-plat-nonsecure INTERFACE nuvoton-vendor-m2351-nonsecure)
