# Global config ---------------------------------------------------------------
project (cascoda-bm)

# Set configuration variables -------------------------------------------------
cascoda_dropdown(CASCODA_BM_INTERFACE
	"The primary interface to use for communications with the host"
	USB UART NONE
)
cascoda_mark_important(CASCODA_BM_INTERFACE)

set( CASCODA_BM_USB_HID_BCDUSBVER 0x10,0x01 CACHE STRING "bcd USB version in form 0x12,0x34")
set( CASCODA_BM_USB_HID_BCDDEVVER 0x00,0x00 CACHE STRING "bcd device version in form 0x12,0x34")
set( CASCODA_BM_USB_HID_IDVENDOR 0x16,0x04 CACHE STRING "USB Vendor ID in form 0x12,0x34")
set( CASCODA_BM_USB_HID_IDPRODUCT 0x20,0x50 CACHE STRING "USB Product ID in form 0x12,0x34")
set( CASCODA_EINK_DISPLAY_PRESENT  OFF CACHE BOOL "Enable half-duplex TX only mode if EINK display is presence")


if(CASCODA_BM_INTERFACE STREQUAL "USB")
	set(USE_USB ON)
	mark_as_advanced(CLEAR CASCODA_BM_USB_HID_BCDUSBVER)
	mark_as_advanced(CLEAR CASCODA_BM_USB_HID_BCDDEVVER)
	mark_as_advanced(CLEAR CASCODA_BM_USB_HID_IDVENDOR)
	mark_as_advanced(CLEAR CASCODA_BM_USB_HID_IDPRODUCT)
else()
	mark_as_advanced(FORCE CASCODA_BM_USB_HID_BCDUSBVER)
	mark_as_advanced(FORCE CASCODA_BM_USB_HID_BCDDEVVER)
	mark_as_advanced(FORCE CASCODA_BM_USB_HID_IDVENDOR)
	mark_as_advanced(FORCE CASCODA_BM_USB_HID_IDPRODUCT)
endif()

if(CASCODA_BM_INTERFACE STREQUAL "UART")
	set(USE_UART ON)
endif()

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/include/cascoda-bm/cascoda-bm-config.h.in"
	"${PROJECT_BINARY_DIR}/include/cascoda-bm/cascoda-bm-config.h"
	)

# Main library config ---------------------------------------------------------
if(CASCODA_EXTERNAL_FLASHCHIP_PRESENT)
	set(EXT_FLASH_SOURCE ${PROJECT_SOURCE_DIR}/source/cascoda_external_flash.c ${PROJECT_SOURCE_DIR}/source/cascoda_ota_upgrade.c)
endif()

add_library(cascoda-bm
	${PROJECT_SOURCE_DIR}/source/cascoda_evbme.c
	${PROJECT_SOURCE_DIR}/source/cascoda_host.c
	${PROJECT_SOURCE_DIR}/source/cascoda_os.c
	${PROJECT_SOURCE_DIR}/source/cascoda_rand.c
	${PROJECT_SOURCE_DIR}/source/cascoda_serial_uart.c
	${PROJECT_SOURCE_DIR}/source/cascoda_serial_usb.c
	${PROJECT_SOURCE_DIR}/source/cascoda_flash.c
	${EXT_FLASH_SOURCE}
	)

if (CASCODA_BUILD_OCF OR UNIX OR MINGW)
	target_sources(cascoda-bm PRIVATE
		${PROJECT_SOURCE_DIR}/source/test15_4_evbme_dummy.c
	)
else()
	target_sources(cascoda-bm PRIVATE
		${PROJECT_SOURCE_DIR}/source/chili_test.c
		${PROJECT_SOURCE_DIR}/source/test15_4_evbme.c
	)
endif()

target_link_libraries(cascoda-bm
	PUBLIC
		ca821x-api
		cascoda-bm-core
		test15-4-api
	PRIVATE
		mbedcrypto
	)

if((CASCODA_CHILI2_CONFIG_STRING STREQUAL "DEV_BOARD") OR (CASCODA_CHILI2_CONFIG_STRING STREQUAL "DEV_BOARD_BATT"))
target_link_libraries(cascoda-bm
	PUBLIC
		cascoda-bm-devboard
	)
endif()

target_include_directories(cascoda-bm
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}/include
	PRIVATE
		${PROJECT_SOURCE_DIR}/source
	)

target_compile_definitions(cascoda-bm
	INTERFACE
		CA_TARGET_NAME=\"$<TARGET_PROPERTY:NAME>\"
)

target_compile_definitions(cascoda-bm PUBLIC CASCODA_OTA_UPGRADE_ENABLED=$<BOOL:${CASCODA_OTA_UPGRADE_ENABLED}>)

target_compile_definitions(cascoda-bm PUBLIC CASCODA_EINK_DISPLAY_PRESENT=$<BOOL:${CASCODA_EINK_DISPLAY_PRESENT}>)


cascoda_use_warnings(cascoda-bm)
