# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (ca821x-openthread)

if (NOT CMAKE_HOST_UNIX)
	message(WARNING "Building Openthread on windows is a challenge. Contributions welcome but it's probably easier to switch to Linux.")
	message(WARNING "Skipping openthread build due to non-unix build system!")
	return()
endif()

# Set configuration variables -------------------------------------------------
set(CASCODA_OPENTHREAD_MAKE make CACHE STRING "The 'make' program to be used to build openthread")
set(CASCODA_OPENTHREAD_MAKE_ARGS -j10 CACHE STRING "The arguments to be passed to the 'make' program when building openthread")
set(CASCODA_OPENTHREAD_CONFIGURE_OPTS
       --enable-cli
       --enable-application-coap
       --enable-mac-filter
       --enable-dhcp6-client
       --enable-dhcp6-server
       --with-mac=external
       --enable-ftd
       --enable-mtd
       --enable-cli
       --disable-docs
     CACHE STRING
       "The arguments to be passed to configure when configuring openthread"
)

set(CASCODA_LOG_LEVELS NONE CRIT WARN NOTE INFO DEBG)
set(CASCODA_LOG_LEVEL CRIT CACHE STRING "The minimum log level to print messages for. eg. NONE, CRIT, WARN, NOTE, INFO, DEBG")
set_property(CACHE CASCODA_LOG_LEVEL PROPERTY STRINGS ${CASCODA_LOG_LEVELS})

if(NOT CASCODA_LOG_LEVEL IN_LIST CASCODA_LOG_LEVELS)
	message(FATAL_ERROR "CASCODA_LOG_LEVEL must be one of ${CASCODA_LOG_LEVELS}")
endif()

# Sub-project configuration ---------------------------------------------------
include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
  openthread
  GIT_REPOSITORY https://github.com/Cascoda/openthread.git
  GIT_TAG        origin/ext-mac
  GIT_SHALLOW    1
)

FetchContent_GetProperties(openthread)
if(NOT openthread_POPULATED)
  FetchContent_Populate(openthread)
endif()

# get the prefix from gcc (openthread seems to be pretty much gcc/clang only)
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)

set(OPENTHREAD_CONFIG_DEFINE OPENTHREAD_PROJECT_CORE_CONFIG_FILE="ca821x-openthread-config.h")
set(OPENTHREAD_INCLUDE_ARG "-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\\\"ca821x-openthread-config.h\\\" -I${PROJECT_BINARY_DIR}/include")
ExternalProject_Add(
  openthread-build
  SOURCE_DIR ${openthread_SOURCE_DIR}
  BINARY_DIR ${openthread_BINARY_DIR}
  CONFIGURE_COMMAND cd ${openthread_SOURCE_DIR} && ./bootstrap && cd ${openthread_SOURCE_DIR}
	    COMMAND ${openthread_SOURCE_DIR}/configure ${CASCODA_OPENTHREAD_CONFIGURE_OPTS}
	    "CXX=${CMAKE_CXX_COMPILER}"
	    "CPP=${CMAKE_C_COMPILER} -E"
	    "CC=${CMAKE_C_COMPILER}"
	    "AR=${CMAKE_C_COMPILER_AR}"
	    "RANLIB=${CMAKE_C_COMPILER_RANLIB}"
	    "NM=${CMAKE_NM}"
	    "STRIP=${CMAKE_STRIP}"
	    "CFLAGS=${CMAKE_C_FLAGS} -Os ${OPENTHREAD_INCLUDE_ARG}"
	    "CPPFLAGS=${CMAKE_C_FLAGS} -Os ${OPENTHREAD_INCLUDE_ARG}"
	    "CXXFLAGS=${CMAKE_CXX_FLAGS} ${OPENTHREAD_INCLUDE_ARG}"
	    "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}"
	    "--host=${MACHINE_NAME}"
  BUILD_COMMAND ${CASCODA_OPENTHREAD_MAKE} ${CASCODA_OPENTHREAD_MAKE_ARGS}
  DOWNLOAD_COMMAND  ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)

add_library(openthread-ftd STATIC IMPORTED GLOBAL)
add_dependencies(openthread-ftd openthread-build)
add_library(openthread-mtd STATIC IMPORTED GLOBAL)
add_dependencies(openthread-mtd openthread-build)
add_library(openthread-cli-ftd STATIC IMPORTED GLOBAL)
add_dependencies(openthread-cli-ftd openthread-build)
add_library(openthread-cli-mtd STATIC IMPORTED GLOBAL)
add_dependencies(openthread-cli-mtd openthread-build)
add_library(openthread-ncp-ftd STATIC IMPORTED GLOBAL)
add_dependencies(openthread-ncp-ftd openthread-build)
add_library(openthread-ncp-mtd STATIC IMPORTED GLOBAL)
add_dependencies(openthread-ncp-mtd openthread-build)
add_library(openthread-mbedtls STATIC IMPORTED)
add_dependencies(openthread-mbedtls openthread-build)
add_library(openthread-api INTERFACE)
add_library(openthread-plat-api INTERFACE)
set_property(TARGET openthread-ftd
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/src/core/${CMAKE_STATIC_LIBRARY_PREFIX}openthread-ftd${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET openthread-mtd
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/src/core/${CMAKE_STATIC_LIBRARY_PREFIX}openthread-mtd${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET openthread-cli-ftd
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/src/cli/${CMAKE_STATIC_LIBRARY_PREFIX}openthread-cli-ftd${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET openthread-cli-mtd
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/src/cli/${CMAKE_STATIC_LIBRARY_PREFIX}openthread-cli-mtd${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET openthread-ncp-ftd
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/src/ncp/${CMAKE_STATIC_LIBRARY_PREFIX}openthread-ncp-ftd${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET openthread-ncp-mtd
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/src/ncp/${CMAKE_STATIC_LIBRARY_PREFIX}openthread-ncp-mtd${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET openthread-mbedtls
             PROPERTY IMPORTED_LOCATION ${openthread_BINARY_DIR}/third_party/mbedtls/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX})

target_include_directories(openthread-api
	INTERFACE
		${openthread_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}/include
)

target_include_directories(openthread-plat-api
	INTERFACE
		${openthread_SOURCE_DIR}/src/core
		${openthread_BINARY_DIR}/include
)

target_link_libraries(openthread-plat-api INTERFACE openthread-api)
target_link_libraries(openthread-ftd INTERFACE openthread-mbedtls openthread-api)
target_link_libraries(openthread-mtd INTERFACE openthread-mbedtls openthread-api)
target_link_libraries(openthread-cli-ftd INTERFACE openthread-ftd)
target_link_libraries(openthread-cli-mtd INTERFACE openthread-mtd)
target_link_libraries(openthread-ncp-ftd INTERFACE openthread-ftd)
target_link_libraries(openthread-ncp-mtd INTERFACE openthread-mtd)

target_compile_definitions(openthread-api
	INTERFACE
		${OPENTHREAD_CONFIG_DEFINE}
	)

# Config file generation ------------------------------------------------------
if(${CASCODA_CA_VER} EQUAL 8210)
	MESSAGE( WARNING "CA-${CASCODA_CA_VER} is not fully supported for thread, please upgrade")
	set(CASCODA_DEVICE_TABLE_SIZE 10)
elseif(${CASCODA_CA_VER} EQUAL 8211)
	set(CASCODA_DEVICE_TABLE_SIZE 32)
else()
	MESSAGE( ERROR "CA-${CASCODA_CA_VER} is not supported with this version")
endif()

configure_file(
	"${PROJECT_SOURCE_DIR}/include/ca821x-openthread-config.h.in"
	"${PROJECT_BINARY_DIR}/include/ca821x-openthread-config.h"
	)

# Run tests -------------------------------------------------------------------
include(CTest)
# TODO: Add tests
