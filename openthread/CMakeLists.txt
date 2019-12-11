# Global config ---------------------------------------------------------------
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
set(CASCODA_OPENTHREAD_DEV OFF CACHE BOOL "Whether to use the latest development build of openthread, or the known stable version.")

#Set default configure options based on platform capabilities
if(CASCODA_OPENTHREAD_FTD)
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
else()
	set(CASCODA_OPENTHREAD_CONFIGURE_OPTS
	       --enable-cli
	       --enable-application-coap
	       --enable-mac-filter
	       --enable-dhcp6-client
	       --enable-dhcp6-server
	       --with-mac=external
	       --enable-ftd
	       --enable-mtd
	       --disable-docs
	     CACHE STRING
	       "The arguments to be passed to configure when configuring openthread"
	)
endif()

# Sub-project configuration ---------------------------------------------------
include(FetchContent)
include(ExternalProject)

# Use specific OpenThread commit, unless developing OpenThread
if(CASCODA_OPENTHREAD_DEV)
	set(OT_TAG ext-mac-dev)
else()
	set(OT_TAG 7efbf601bd9395b70c1f658a2dd8c9bc7ddd943e)
endif()

FetchContent_Declare(
  openthread
  GIT_REPOSITORY https://github.com/Cascoda/openthread.git
  GIT_TAG        ${OT_TAG}
  GIT_SHALLOW    1
)

FetchContent_GetProperties(openthread)
if(NOT openthread_POPULATED)
  FetchContent_Populate(openthread)
endif()

# get the prefix from gcc (openthread seems to be pretty much gcc/clang only)
if(CASCODA_MACHINE_NAME)
	set(MACHINE_NAME ${CASCODA_MACHINE_NAME})
else()
	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
message("Target machine: ${MACHINE_NAME}")

# get an uppercase version of the build type, for extracting build_type specific flags
if(CMAKE_BUILD_TYPE)
	string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UC)
endif()

if(TARGET openthread-mbedtls-plat)
	get_target_property(MBEDTLS_DEFINITIONS openthread-mbedtls-plat INTERFACE_COMPILE_DEFINITIONS)
	#TODO: This will only work with compilers that support -Ddefine=definition syntax
	string(REPLACE \" \\\" MBEDTLS_DEFINITIONS "${MBEDTLS_DEFINITIONS}")
	list(TRANSFORM MBEDTLS_DEFINITIONS PREPEND -D)
	get_target_property(MBEDTLS_INCLUDES openthread-mbedtls-plat INTERFACE_INCLUDE_DIRECTORIES)
	list(TRANSFORM MBEDTLS_INCLUDES PREPEND -I)
	string(REPLACE ";" " " MBEDTLS_INCLUDES "${MBEDTLS_INCLUDES}")
	string(REPLACE ";" " " MBEDTLS_DEFINITIONS "${MBEDTLS_DEFINITIONS}")
endif()
set(MBEDTLS_DEFINITIONS "${MBEDTLS_DEFINITIONS} -DMBEDTLS_CONFIG_FILE=\\\"mbedtls-config.h\\\" -I${openthread_SOURCE_DIR}/third_party/mbedtls/repo/include -I${openthread_SOURCE_DIR}/third_party/mbedtls ${MBEDTLS_INCLUDES}")

set(OPENTHREAD_CONFIG_DEFINE OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"ca821x-openthread-config.h\")
set(OPENTHREAD_INCLUDE_ARG "-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\\\"ca821x-openthread-config.h\\\" -I${PROJECT_BINARY_DIR}/include")
ExternalProject_Add(
  openthread-build
  SOURCE_DIR ${openthread_SOURCE_DIR}
  BINARY_DIR ${openthread_BINARY_DIR}
  CONFIGURE_COMMAND cd ${openthread_SOURCE_DIR}
	    COMMAND ${openthread_SOURCE_DIR}/configure ${CASCODA_OPENTHREAD_CONFIGURE_OPTS}
	    "CXX=${CMAKE_CXX_COMPILER}"
	    "CPP=${CMAKE_C_COMPILER} -E"
	    "CC=${CMAKE_C_COMPILER}"
	    "AR=${CMAKE_C_COMPILER_AR}"
	    "RANLIB=${CMAKE_C_COMPILER_RANLIB}"
	    "NM=${CMAKE_NM}"
	    "STRIP=${CMAKE_STRIP}"
	    "CFLAGS=${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${BUILD_TYPE_UC}} ${OPENTHREAD_INCLUDE_ARG}"
	    "CPPFLAGS=${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${BUILD_TYPE_UC}} ${OPENTHREAD_INCLUDE_ARG}"
	    "CXXFLAGS=${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BUILD_TYPE_UC}} ${OPENTHREAD_INCLUDE_ARG}"
	    "MBEDTLS_CPPFLAGS=${MBEDTLS_DEFINITIONS} "
	    "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${BUILD_TYPE_UC}}"
	    "--host=${MACHINE_NAME}"
  BUILD_COMMAND ${CASCODA_OPENTHREAD_MAKE} ${CASCODA_OPENTHREAD_MAKE_ARGS}
  BUILD_ALWAYS ON
  DOWNLOAD_COMMAND  cd ${openthread_SOURCE_DIR} && ./bootstrap
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
add_library(openthread-mbedtls STATIC IMPORTED GLOBAL)
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

#Workaround to stop cmake complaining about include dir that doesn't exist at config time
file (MAKE_DIRECTORY ${openthread_BINARY_DIR}/include)

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

target_include_directories(openthread-mbedtls
	INTERFACE
		${openthread_SOURCE_DIR}/third_party/mbedtls/repo/include
		${openthread_SOURCE_DIR}/third_party/mbedtls
)

target_link_libraries(openthread-plat-api INTERFACE openthread-api)
target_link_libraries(openthread-ftd INTERFACE openthread-mbedtls openthread-api)
target_link_libraries(openthread-mtd INTERFACE openthread-mbedtls openthread-api)
target_link_libraries(openthread-cli-ftd INTERFACE openthread-ftd)
target_link_libraries(openthread-cli-mtd INTERFACE openthread-mtd)
target_link_libraries(openthread-ncp-ftd INTERFACE openthread-ftd)
target_link_libraries(openthread-ncp-mtd INTERFACE openthread-mtd)
target_link_libraries(openthread-mbedtls INTERFACE openthread-plat-api)

if(TARGET openthread-mbedtls-plat)
	target_link_libraries(openthread-mbedtls INTERFACE openthread-mbedtls-plat)
endif()

target_compile_definitions(openthread-api
	INTERFACE
		${OPENTHREAD_CONFIG_DEFINE}
)

target_compile_definitions(openthread-mbedtls
	INTERFACE
		MBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"
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
