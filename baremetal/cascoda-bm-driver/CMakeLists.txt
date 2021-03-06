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
add_library(cascoda-bm
	${PROJECT_SOURCE_DIR}/source/cascoda_evbme.c
	${PROJECT_SOURCE_DIR}/source/cascoda_host.c
	${PROJECT_SOURCE_DIR}/source/cascoda_os.c
	${PROJECT_SOURCE_DIR}/source/cascoda_rand.c
	${PROJECT_SOURCE_DIR}/source/cascoda_serial_uart.c
	${PROJECT_SOURCE_DIR}/source/cascoda_serial_usb.c
	)

target_link_libraries(cascoda-bm
	PUBLIC
		ca821x-api
		cascoda-bm-core
	PRIVATE
		mbedcrypto
	)

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

cascoda_use_warnings(cascoda-bm)

