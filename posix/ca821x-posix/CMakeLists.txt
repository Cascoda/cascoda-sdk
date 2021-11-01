# Global config ---------------------------------------------------------------
project (ca821x-posix)

# Get required packages
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Mark the device as being FTD-capable
set(CASCODA_OPENTHREAD_FTD ON CACHE INTERNAL "")

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
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-evbme.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-log.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-rand.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-util.c
		${PROJECT_SOURCE_DIR}/source/util/cascoda-flash.c
		)

	target_link_libraries(ca821x-posix PUBLIC
		cascoda-util
		Threads::Threads
		${CMAKE_DL_LIBS}
	)
endif()

if(WIN32)
	add_library(ca821x-posix
		${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-generic-exchange.c
		${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-queue.c
		# For the moment, Windows supports only the USB exchange
		# ${PROJECT_SOURCE_DIR}/source/kernel-exchange/kernel-exchange.c
		# ${PROJECT_SOURCE_DIR}/source/uart-exchange/uart-exchange.c
		${PROJECT_SOURCE_DIR}/source/usb-exchange/usb-exchange.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-evbme.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-log.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-rand.c
		${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-util.c
		${PROJECT_SOURCE_DIR}/source/util/cascoda-flash.c
		)

	target_link_libraries(ca821x-posix PUBLIC
		cascoda-util
		Threads::Threads
		hidapi
	)

	target_link_options(ca821x-posix PUBLIC -static)
endif()

target_include_directories(ca821x-posix
	PRIVATE
		${PROJECT_SOURCE_DIR}/source/generic-exchange
		${PROJECT_SOURCE_DIR}/source/kernel-exchange
		${PROJECT_SOURCE_DIR}/source/uart-exchange
		${PROJECT_SOURCE_DIR}/source/usb-exchange
		${PROJECT_SOURCE_DIR}/source/util
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

