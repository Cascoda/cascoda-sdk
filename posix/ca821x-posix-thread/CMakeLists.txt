# Global config ---------------------------------------------------------------
project (ca821x-posix-thread)

if (WIN32)
	message(WARNING "Openthread not building for windows - platform layer not currently compatible.")
	return()
endif()

# Get required packages -------------------------------------------------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Main library config ---------------------------------------------------------
add_library(ca821x-openthread-posix-plat
	${PROJECT_SOURCE_DIR}/platform/alarm.c
	${PROJECT_SOURCE_DIR}/platform/entropy.c
	${PROJECT_SOURCE_DIR}/platform/flash.c
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

target_compile_definitions(ca821x-openthread-posix-plat PRIVATE ${OPENTHREAD_CONFIG_DEFINE})

target_include_directories( ca821x-openthread-posix-plat
	PRIVATE
		${PROJECT_SOURCE_DIR}/platform
	PUBLIC
		${PROJECT_SOURCE_DIR}/platform/include
		${PROJECT_BINARY_DIR}/platform/include
	)

# Add helper interface libraries to simplify the cyclic dependency between
# the platform layer and the openthread core.
add_library(ca821x-openthread-posix-ftd INTERFACE)
target_link_libraries(ca821x-openthread-posix-ftd
	INTERFACE
		openthread-ftd
		ca821x-openthread-posix-plat
		openthread-ftd
		ca821x-openthread-posix-plat
	)

add_library(ca821x-openthread-posix-mtd INTERFACE)
target_link_libraries(ca821x-openthread-posix-mtd
	INTERFACE
		openthread-mtd
		ca821x-openthread-posix-plat
		openthread-mtd
		ca821x-openthread-posix-plat
	)

# Test app config -------------------------------------------------------------
add_executable(cliapp
	${PROJECT_SOURCE_DIR}/example/main.c
	)

add_executable(cliapp-mtd
	${PROJECT_SOURCE_DIR}/example/main.c
	)

add_executable(ncpapp
	${PROJECT_SOURCE_DIR}/example/ncpapp.c
	)

add_executable(mt-example
	${PROJECT_SOURCE_DIR}/example/mainMultithread.c
	)

add_executable(ot-server-standalone
	${PROJECT_SOURCE_DIR}/example/serverStandalone.c
	)

add_executable(ot-server-eink
	${PROJECT_SOURCE_DIR}/example/serverEink.c
	)

target_link_libraries(cliapp openthread-cli-ftd ca821x-openthread-posix-ftd)
target_link_libraries(cliapp-mtd openthread-cli-mtd ca821x-openthread-posix-mtd)
target_link_libraries(ncpapp openthread-ncp-ftd ca821x-openthread-posix-ftd)
target_link_libraries(mt-example openthread-cli-ftd ca821x-openthread-posix-ftd Threads::Threads)
target_link_libraries(ot-server-standalone ca821x-openthread-posix-ftd tinycbor-master)
target_link_libraries(ot-server-eink ca821x-openthread-posix-ftd)

install(
	TARGETS
		cliapp
		cliapp-mtd
		ncpapp
		mt-example
		ot-server-standalone
		ot-server-eink
	COMPONENT
		examples
	RUNTIME DESTINATION
		${CMAKE_INSTALL_BINDIR}
)
