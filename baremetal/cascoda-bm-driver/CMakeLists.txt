# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (cascoda-bm)

# Set configuration variables -------------------------------------------------
option( CASCODA_BM_USE_USB "Enable baremetal USB support" ON)
set( CASCODA_BM_USB_HID_BCDUSBVER 0x10,0x01 CACHE STRING "bcd USB version in form 0x12,0x34")
set( CASCODA_BM_USB_HID_BCDDEVVER 0x00,0x00 CACHE STRING "bcd device version in form 0x12,0x34")
set( CASCODA_BM_USB_HID_IDVENDOR 0x16,0x04 CACHE STRING "USB Vendor ID in form 0x12,0x34")
set( CASCODA_BM_USB_HID_IDPRODUCT 0x20,0x50 CACHE STRING "USB Product ID in form 0x12,0x34")
option( CASCODA_BM_USE_UART "Enable baremetal UART support" OFF)

if(CASCODA_BM_USE_USB)
	set(USE_USB ON)
endif()

if(CASCODA_BM_USE_USB)
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

if(CASCODA_BM_USE_UART)
	set(USE_UART ON)
endif()

if(USE_UART AND USE_USB)
	message(SEND_ERROR "Can only use UART or USB in CASCODA_BM, not both")
endif()

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/include/cascoda-bm/cascoda-bm-config.h.in"
	"${PROJECT_BINARY_DIR}/include/cascoda-bm/cascoda-bm-config.h"
	)

# Main library config ---------------------------------------------------------
add_library(cascoda-bm
	${PROJECT_SOURCE_DIR}/source/cascoda_host.c
	${PROJECT_SOURCE_DIR}/source/cascoda_evbme.c
	${PROJECT_SOURCE_DIR}/source/cascoda_serial_uart.c
	${PROJECT_SOURCE_DIR}/source/cascoda_serial_usb.c
	${PROJECT_SOURCE_DIR}/source/cascoda_spi.c
	${PROJECT_SOURCE_DIR}/source/cascoda_time.c
	${PROJECT_SOURCE_DIR}/source/cascoda_usbhid.c
	${PROJECT_SOURCE_DIR}/source/cascoda_wait.c
	)

target_link_libraries(cascoda-bm
	PUBLIC
		ca821x-api
	INTERFACE
		cascoda-bm-plat
	)

target_include_directories( cascoda-bm
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}/include
	)

cascoda_use_warnings(cascoda-bm)

# Add tests -------------------------------------------------------------------
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
