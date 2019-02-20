# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (cascoda-nuvoton-chili)

# Return if not suitable for system processor
if(NOT (CMAKE_SYSTEM_PROCESSOR STREQUAL "NANO120"))
	return()
endif()

# Return if platform already configured
if(TARGET cascoda-bm-plat)
	return()
endif()

# define the 'interface' library that makes up the bm platform.
add_library(cascoda-bm-plat INTERFACE)

# Set configuration variables -------------------------------------------------
set(CASCODA_CHILI_REVS 2 3)
set(CASCODA_CHILI_REV 3 CACHE STRING "The revision of the chili hardware being used 1.x (1.2 is green, 1.3 is red)")
set_property(CACHE CASCODA_CHILI_REV PROPERTY STRINGS ${CASCODA_CHILI_REVS})

if(NOT CASCODA_CHILI_REV IN_LIST CASCODA_CHILI_REVS)
    message(FATAL_ERROR "CASCODA_CHILI_REV must be one of ${CASCODA_CHILI_REVS}")
endif()

set(CASCODA_CHILI_FLASH_PAGES 2 CACHE STRING "The number of 512 byte flash pages that should be used for user flash")

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/port/include/cascoda_chili_config.h.in"
	"${PROJECT_BINARY_DIR}/port/include/cascoda_chili_config.h"
	)

# Sub-project configuration ---------------------------------------------------
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor ${PROJECT_BINARY_DIR}/vendor)

# Main library config ---------------------------------------------------------
add_library(cascoda-chili
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_dataflash_nano120.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_debug_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_hid_nano120.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_nano120.c
	$<TARGET_OBJECTS:nuvoton-vendor-nano120>
	)

target_link_libraries(cascoda-chili
	PUBLIC
		cascoda-bm
	)

# Add linker file dependancy
if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
	target_link_options(cascoda-chili
		PUBLIC
			"-T${PROJECT_SOURCE_DIR}/nano120BN.ld"
		)
endif()

target_include_directories( cascoda-chili
	PUBLIC
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
	)

# Export the chili platform as cascoda-bm-plat
target_link_libraries(cascoda-bm-plat INTERFACE cascoda-chili nuvoton-vendor-nano120)

# Run tests -------------------------------------------------------------------
include(CTest)
# TODO: Add tests
