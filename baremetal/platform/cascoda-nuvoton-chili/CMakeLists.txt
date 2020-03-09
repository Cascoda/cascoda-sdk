# Global config ---------------------------------------------------------------
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
cascoda_dropdown(CASCODA_CHILI_REV
	"The revision of the chili hardware being used 1.x (1.2 is green, 1.3 is red)"
	3 2
	)

set(CASCODA_CHILI_FLASH_PAGES 4 CACHE STRING "The number of 512 byte flash pages that should be used for user flash")

cascoda_mark_important("CASCODA_CHILI_FLASH_PAGES")
# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/port/include/cascoda_chili_config.h.in"
	"${PROJECT_BINARY_DIR}/port/include/cascoda_chili_config.h"
	)

# Sub-project configuration ---------------------------------------------------
add_subdirectory(vendor)

# Main library config ---------------------------------------------------------
add_library(cascoda-chili
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_dataflash_nano120.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_gpio_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_isr_chili.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_usb_nano120.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_sensorif_nano120.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_time.c
	$<TARGET_OBJECTS:nuvoton-vendor-nano120>
	)

target_link_libraries(cascoda-chili
	PUBLIC
		cascoda-bm
		nuvoton-vendor-nano120
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
target_link_libraries(cascoda-bm-plat INTERFACE cascoda-chili)

# Variables for configuring memory usage using the cascoda_configure_memory CMake function
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set(CASCODA_STARTUP_ASM ${CMAKE_SOURCE_DIR}/baremetal/platform/cascoda-nuvoton-chili/vendor/Nano100Library/Device/Nuvoton/Nano100Series/Source/GCC/startup_Nano100Series.S CACHE INTERNAL "")
	set(CASCODA_STACK_SIZE __STACK_SIZE CACHE INTERNAL "")
	set(CASCODA_HEAP_SIZE __HEAP_SIZE CACHE INTERNAL "")
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CASCODA_STARTUP_ASM ${CMAKE_SOURCE_DIR}/baremetal/platform/cascoda-nuvoton-chili/vendor/Nano100Library/Device/Nuvoton/Nano100Series/Source/ARM/startup_Nano100Series.s CACHE INTERNAL "")
	set(CASCODA_STACK_SIZE Stack_Size CACHE INTERNAL "")
	set(CASCODA_HEAP_SIZE Heap_Size CACHE INTERNAL "")
endif()
