# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (ca821x-posix)

# Get required packages
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Set configuration variables -------------------------------------------------
option( CA821X_ASYNC_CALLBACK
	"Enable asynchronous calling of user callbacks from another thread (vs polling using ca821x_util_dispatch_poll)"
	ON
)

# Mark the device as being FTD-capable
set(CASCODA_OPENTHREAD_FTD ON PARENT_SCOPE)

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/include/ca821x-posix/ca821x-posix-config.h.in"
	"${PROJECT_BINARY_DIR}/include/ca821x-posix/ca821x-posix-config.h"
	)

# Sub-project configuration ---------------------------------------------------
include(FetchContent)

# TODO: allow system version of hidapi if already installed & improve this embedding
FetchContent_Declare(
  hidapi
  GIT_REPOSITORY https://github.com/signal11/hidapi.git
  GIT_TAG        a6a622ffb680c55da0de787ff93b80280498330f
  GIT_SHALLOW    1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)

FetchContent_GetProperties(hidapi)
if(NOT hidapi_POPULATED)
  FetchContent_Populate(hidapi)
endif()

# Main library config ---------------------------------------------------------
add_library(ca821x-posix
	${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-generic-exchange.c
	${PROJECT_SOURCE_DIR}/source/generic-exchange/ca821x-queue.c
	${PROJECT_SOURCE_DIR}/source/kernel-exchange/kernel-exchange.c
	${PROJECT_SOURCE_DIR}/source/uart-exchange/uart-exchange.c
	${PROJECT_SOURCE_DIR}/source/usb-exchange/usb-exchange.c
	${PROJECT_SOURCE_DIR}/source/util/ca821x-posix-util.c
	)

target_link_libraries(ca821x-posix ca821x-api Threads::Threads ${CMAKE_DL_LIBS})

target_include_directories( ca821x-posix
	PRIVATE
		${PROJECT_SOURCE_DIR}/source/generic-exchange
		${PROJECT_SOURCE_DIR}/source/kernel-exchange
		${PROJECT_SOURCE_DIR}/source/uart-exchange
		${PROJECT_SOURCE_DIR}/source/usb-exchange
		${hidapi_SOURCE_DIR}
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}/include
	)
	cascoda_use_warnings(ca821x-posix)

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

add_executable(serial-adapter
	${PROJECT_SOURCE_DIR}/example/serial-adapter.c
	)

target_link_libraries(example_app ca821x-api ca821x-posix)
target_link_libraries(security_test ca821x-api ca821x-posix)
target_link_libraries(usb_test ca821x-api ca821x-posix)
target_link_libraries(serial-adapter ca821x-posix)
