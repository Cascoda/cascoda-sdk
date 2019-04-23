# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (ca821x-posix-thread)

if (NOT CMAKE_HOST_UNIX)
	message(WARNING "Building Openthread on windows is a challenge. Contributions welcome but it's probably easier to switch to Linux.")
	message(WARNING "Skipping openthread build due to non-unix build system!")
	return()
endif()

# Get required packages -------------------------------------------------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Set configuration variables -------------------------------------------------
set(CASCODA_OPENTHREAD_CONFIGURE_OPTS
       --enable-application-coap
       --enable-joiner
       --enable-commissioner
       --enable-tmf-proxy
       --enable-mac-filter
       --enable-cert-log
       --enable-dhcp6-client
       --enable-dhcp6-server
       --enable-border-router
       --with-mac=external
       --enable-ftd
       --enable-mtd
       --enable-cli
       --enable-ncp
       --with-ncp-bus=uart
       --disable-docs
     CACHE STRING
       "The arguments to be passed to configure when configuring openthread"
)


# Main library config ---------------------------------------------------------
add_library(ca821x-openthread-posix-plat
	${PROJECT_SOURCE_DIR}/platform/alarm.c
	${PROJECT_SOURCE_DIR}/platform/flash.c
	${PROJECT_SOURCE_DIR}/platform/logging.c
	${PROJECT_SOURCE_DIR}/platform/misc.c
	${PROJECT_SOURCE_DIR}/platform/platform.c
	${PROJECT_SOURCE_DIR}/platform/radio.c
	${PROJECT_SOURCE_DIR}/platform/radio-stubs.c
	${PROJECT_SOURCE_DIR}/platform/random.c
	${PROJECT_SOURCE_DIR}/platform/selfpipe.c
	${PROJECT_SOURCE_DIR}/platform/serial.c
	${PROJECT_SOURCE_DIR}/platform/settings.c
	${PROJECT_SOURCE_DIR}/platform/spi-stubs.c
	)

add_dependencies(ca821x-openthread-posix-plat openthread-build)

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

# Add helper interface libraries to simplify the cyclic dependancy between
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

target_link_libraries(cliapp openthread-cli-ftd ca821x-openthread-posix-ftd)
target_link_libraries(cliapp-mtd openthread-cli-mtd ca821x-openthread-posix-mtd)
target_link_libraries(ncpapp openthread-ncp-ftd ca821x-openthread-posix-ftd)
target_link_libraries(mt-example openthread-cli-ftd ca821x-openthread-posix-ftd Threads::Threads)
target_link_libraries(ot-server-standalone ca821x-openthread-posix-ftd)
