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
set(CASCODA_CHILI2_REVS 0 -1)
set(CASCODA_CHILI2_REV 0 CACHE STRING "The revision of the chili 2 hardware being used 2.x (2.0 is chili, use -1 for numaker devboard)")
set_property(CACHE CASCODA_CHILI2_REV PROPERTY STRINGS ${CASCODA_CHILI2_REVS})

if(NOT CASCODA_CHILI2_REV IN_LIST CASCODA_CHILI2_REVS)
    message(FATAL_ERROR "CASCODA_CHILI2_REV must be one of ${CASCODA_CHILI2_REVS}")
endif()

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
	${PROJECT_SOURCE_DIR}/port/source/cascoda_dataflash_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_debug_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_hid_m2351.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_m2351.c
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
endif()

target_include_directories( cascoda-chili2
	PUBLIC
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
	)

# Export the chili2 platform as cascoda-bm-plat
target_link_libraries(cascoda-bm-plat INTERFACE cascoda-chili2)

# Run tests -------------------------------------------------------------------
include(CTest)
# TODO: Add tests
