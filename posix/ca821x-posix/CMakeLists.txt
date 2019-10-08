# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (ca821x-posix)

# Get required packages
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Mark the device as being FTD-capable
set(CASCODA_OPENTHREAD_FTD ON PARENT_SCOPE)

# Option for the Raspberry pi USB workaround, with crude autodetection
if(UNIX
	AND CMAKE_HOST_UNIX
	AND NOT CASCODA_ENABLE_RASPI_USB_WORKAROUND
	AND EXISTS "/sys/bus/usb/devices/usb1/manufacturer")
	file(READ "/sys/bus/usb/devices/usb1/manufacturer" USB_ROOT_INFO)
	if(USB_ROOT_INFO MATCHES "dwc_otg_hcd")
		set(CASCODA_ENABLE_RASPI_USB_WORKAROUND ON)
	endif()
endif()
option( CASCODA_RASPI_USB_WORKAROUND
	"Enable the raspberry pi USB workaround which keeps the connection active"
	${CASCODA_ENABLE_RASPI_USB_WORKAROUND})

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/include/ca821x-posix/ca821x-posix-config.h.in"
	"${PROJECT_BINARY_DIR}/include/ca821x-posix/ca821x-posix-config.h"
	)

# Main library config ---------------------------------------------------------
if(UNIX)
	add_library(ca821x-posix
		${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-generic-exchange.c
		${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-queue.c
		${PROJECT_SOURCE_DIR}/source/kernel-exchange/kernel-exchange.c
		${PROJECT_SOURCE_DIR}/source/uart-exchange/uart-exchange.c
		${PROJECT_SOURCE_DIR}/source/usb-exchange/usb-exchange.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-log.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-util.c
		)

	target_link_libraries(ca821x-posix PUBLIC ca821x-api Threads::Threads ${CMAKE_DL_LIBS})
endif()

if(WIN32)
	add_library(ca821x-posix
		${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-generic-exchange.c
		${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-queue.c
		# For the moment, Windows supports only the USB exchange
		# ${PROJECT_SOURCE_DIR}/source/kernel-exchange/kernel-exchange.c
		# ${PROJECT_SOURCE_DIR}/source/uart-exchange/uart-exchange.c
		${PROJECT_SOURCE_DIR}/source/usb-exchange/usb-exchange.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-log.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-util.c
		)

	target_link_libraries(ca821x-posix PUBLIC
		ca821x-api
		Threads::Threads
		hidapi
	)
endif()

target_include_directories(ca821x-posix
	PRIVATE
		${PROJECT_SOURCE_DIR}/source/generic-exchange
		${PROJECT_SOURCE_DIR}/source/kernel-exchange
		${PROJECT_SOURCE_DIR}/source/uart-exchange
		${PROJECT_SOURCE_DIR}/source/usb-exchange
		$<TARGET_PROPERTY:hidapi,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}/include
	)
cascoda_use_warnings(ca821x-posix)

# Real Time library is needed to use the clock_gettime functions
if(CASCODA_RASPI_USB_WORKAROUND)
	target_link_libraries(ca821x-posix PRIVATE rt)
endif()

# Test app config -------------------------------------------------------------
add_executable(example_app
	${PROJECT_SOURCE_DIR}/example/main.c
	)

add_executable(security_test
	${PROJECT_SOURCE_DIR}/example/security-test.c
	)

add_executable(usb_test
	${PROJECT_SOURCE_DIR}/example/usb-test.c
	)

target_link_libraries(example_app ca821x-api ca821x-posix)
target_link_libraries(security_test ca821x-api ca821x-posix)
target_link_libraries(usb_test ca821x-api ca821x-posix)

# For the moment, serial-adapter is Linux-only
if(UNIX)
	add_executable(serial-adapter
		${PROJECT_SOURCE_DIR}/example/serial-adapter.c
	)
	target_link_libraries(serial-adapter ca821x-posix)
endif()
