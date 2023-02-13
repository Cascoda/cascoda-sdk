# Global config ---------------------------------------------------------------
project (ca821x-posix-thread)

if (WIN32)
	message(WARNING "Openthread not building for windows - platform layer not currently compatible.")
	return()
endif()

# Set the message buffers pretty high as we generally have plenty of RAM on posix
set(CASCODA_OPENTHREAD_MESSAGE_BUFS 200 CACHE STRING "Integer number of message buffers for openthread to use. Recommend >44")

# Get required packages -------------------------------------------------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Main library config ---------------------------------------------------------
add_library(ca821x-openthread-posix-plat OBJECT
	${PROJECT_SOURCE_DIR}/platform/alarm.c
	${PROJECT_SOURCE_DIR}/platform/entropy.c
	${PROJECT_SOURCE_DIR}/platform/logging.c
	${PROJECT_SOURCE_DIR}/platform/misc.c
	${PROJECT_SOURCE_DIR}/platform/platform.c
	${PROJECT_SOURCE_DIR}/platform/radio.c
	${PROJECT_SOURCE_DIR}/platform/radio-stubs.c
	${PROJECT_SOURCE_DIR}/platform/selfpipe.c
	${PROJECT_SOURCE_DIR}/platform/serial.c
	${PROJECT_SOURCE_DIR}/platform/settings.c
	${PROJECT_SOURCE_DIR}/platform/spi-stubs.c
	)

target_link_libraries(ca821x-openthread-posix-plat
	PUBLIC
		ca821x-posix
	PRIVATE
		m
		openthread-plat-api
	)

target_include_directories(ca821x-openthread-posix-plat
	PRIVATE
		${PROJECT_SOURCE_DIR}/platform
	PUBLIC
		${PROJECT_SOURCE_DIR}/platform/include
		${PROJECT_BINARY_DIR}/platform/include
	)

# Create the user libraries that are actually used by applications
add_library(ca821x-openthread-posix-ftd
	$<TARGET_OBJECTS:ca821x-openthread-posix-plat>
	$<TARGET_OBJECTS:openthread-ftd-obj>
	$<TARGET_OBJECTS:ca-ot-util>
	)
target_link_libraries(ca821x-openthread-posix-ftd
	PUBLIC
		ca-ot-util
		openthread-ftd-obj
		ca821x-openthread-posix-plat
	)

add_library(ca821x-openthread-posix-mtd
	$<TARGET_OBJECTS:ca821x-openthread-posix-plat>
	$<TARGET_OBJECTS:openthread-mtd-obj>
	$<TARGET_OBJECTS:ca-ot-util>
	)
target_link_libraries(ca821x-openthread-posix-mtd
	PUBLIC
		ca-ot-util
		openthread-mtd-obj
		ca821x-openthread-posix-plat
	)

# Test app config -------------------------------------------------------------
add_executable(ot-cli-posix-ftd
	${PROJECT_SOURCE_DIR}/example/main.c
	)

add_executable(ot-cli-posix-mtd
	${PROJECT_SOURCE_DIR}/example/main.c
	)

target_link_libraries(ot-cli-posix-ftd openthread-cli-ftd ca821x-openthread-posix-ftd)
target_link_libraries(ot-cli-posix-mtd openthread-cli-mtd ca821x-openthread-posix-mtd)

install(
	TARGETS
		ot-cli-posix-ftd
		ot-cli-posix-mtd
	COMPONENT
		examples
	RUNTIME DESTINATION
		bin
)
