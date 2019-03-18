# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.12)
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

cascoda_dropdown(CASCODA_CHILI2_CONFIG_STRING
	"The hardware configuration of the Chili 2"
	TWO_SIDED ONE_SIDED
	)
cascoda_map(${CASCODA_CHILI2_CONFIG_STRING} CASCODA_CHILI2_CONFIG
	ONE_SIDED 0
	TWO_SIDED 1
	)

set(CASCODA_CHILI_FLASH_PAGES 2 CACHE STRING "The number of 2048 byte flash pages that should be used for user flash")

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/port/include/cascoda_chili_config.h.in"
	"${PROJECT_BINARY_DIR}/port/include/cascoda_chili_config.h"
	)

# Sub-project configuration ---------------------------------------------------

add_subdirectory(${PROJECT_SOURCE_DIR}/vendor ${PROJECT_BINARY_DIR}/vendor)

# Main library config ---------------------------------------------------------
add_library(cascoda-chili2
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_dataflash_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_debug_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_sensorif_m2351.c
	$<TARGET_OBJECTS:nuvoton-vendor-m2351>
	)

target_link_libraries(cascoda-chili2
	PUBLIC
		cascoda-bm
	PRIVATE
		nuvoton-vendor-m2351
	)

# Add linker file dependancy
if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
	target_link_options(cascoda-chili2
		PUBLIC
			"-T${PROJECT_SOURCE_DIR}/m2351BN.ld"
		)
elseif(CMAKE_C_COMPILER MATCHES armclang)
	target_link_options(cascoda-chili2
		PUBLIC
			"-Wl,--ro-base=0x00000000,--entry=0x00000000,--rw-base=0x20000000,--entry=Reset_Handler,--first=__Vectors,--strict,--datacompressor=off,--entry=Reset_Handler,--library_type=microlib"
		)
endif()

target_include_directories( cascoda-chili2
	PUBLIC
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
	)

cascoda_use_warnings(cascoda-chili2)

# Export the chili2 platform as cascoda-bm-plat
target_link_libraries(cascoda-bm-plat INTERFACE cascoda-chili2)
