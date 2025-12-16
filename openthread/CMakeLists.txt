# Global config ---------------------------------------------------------------
project (ca821x-openthread)

if(NOT (WIN32 OR UNIX))
	set(CASCODA_BUILD_OT ON CACHE BOOL "Whether to build OpenThread" FORCE)
endif()

if(NOT CASCODA_BUILD_OT)
	return()
endif()

# Set configuration variables -------------------------------------------------
set(CASCODA_OPENTHREAD_DEV OFF CACHE BOOL "Whether to use the latest development build of openthread, or the known stable version.")
set(CASCODA_OPENTHREAD_MESSAGE_BUFS 44 CACHE STRING "Integer number of message buffers for openthread to use. Recommend >44")

if(${CASCODA_CA_VER} EQUAL 8210)
	MESSAGE( WARNING "CA-${CASCODA_CA_VER} is not fully supported for thread, please upgrade")
	set(CASCODA_DEVICE_TABLE_SIZE 10)
elseif(${CASCODA_CA_VER} EQUAL 8211)
	set(CASCODA_DEVICE_TABLE_SIZE 32)
elseif(${CASCODA_CA_VER} EQUAL 8212)
	set(CASCODA_DEVICE_TABLE_SIZE 32)
else()
	MESSAGE( ERROR "CA-${CASCODA_CA_VER} is not supported with this version of cascoda SDK")
endif()

if(${CASCODA_BUILD_OCF})
       set(OT_EXTERNAL_MBEDTLS "mbedtls" CACHE STRING "Specify external mbedtls library" FORCE)
else()
       unset(OT_EXTERNAL_MBEDTLS CACHE)
endif()

mark_as_advanced(OT_EXTERNAL_MBEDTLS)

# Sub-project configuration ---------------------------------------------------
include(FetchContent)

# Use specific OpenThread commit, unless developing OpenThread
if(CASCODA_OPENTHREAD_DEV)
	set(OT_TAG thread-testcase-6.1.1-sed)
#	set(OT_TAG ext-mac-dev)
else()
	set(OT_TAG main)
	#set(OT_TAG main)
	# This is the old OpenThread commit, from before Elie's OpenThread update.
	# It has not been tested with the changes to the Thread platform, and it is
	# therefore deprecated.
	# set(OT_TAG 8b4518581e4fd4d3daebd03ac264ebe8eee1bfe3)
endif()

# Only set the openthread repo if it is not already set (user can point to mirror)
if(NOT CASCODA_OPENTHREAD_REPO)
    set(CASCODA_OPENTHREAD_REPO "https://$ENV{OPENTHREAD_PRIVATE}@github.com/Cascoda/openthread.git")
endif()

FetchContent_Declare(
  openthread
  GIT_REPOSITORY ${CASCODA_OPENTHREAD_REPO}
  GIT_TAG        ${OT_TAG}
)

set(OT_BUILD_EXECUTABLES OFF CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_PLATFORM "external" CACHE STRING "Needed for the Cascoda SDK - please do not change")
set(OT_EXCLUDE_TCPLP_LIB ON CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_MAC_FILTER OFF CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_THREAD_VERSION "1.1" CACHE STRING "Needed for the Cascoda SDK - please do not change")
set(OT_ASSERT ON CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_BORDER_AGENT "OFF" CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_BORDER_ROUTER "OFF" CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_DHCP_SERVER "OFF" CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_SERVICE "OFF" CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_ECDSA "ON" CACHE BOOL "Needed for the Cascoda SDK - please do not change")
set(OT_SRP_CLIENT "ON" CACHE BOOL "Needed for the Cascoda SDK - please do not change")

set(CASCODA_MAX_EXT_MCAST_ADDR "7" CACHE STRING "Number of external multicast addresses stored on this device")
mark_as_advanced(CASCODA_MAX_EXT_MCAST_ADDR)

if(CASCODA_OPENTHREAD_FTD)
	option(OT_BORDER_AGENT "enable border agent support" ON)
	option(OT_BORDER_ROUTER "enable border router support" ON)
	option(OT_COMMISSIONER "enable commissioner support" ON)
	option(OT_JOINER "enable joiner support" ON)
	option(OT_SERVICE "enable support for injecting Service entries into the Thread Network Data" ON)
	option(OT_DHCP6_SERVER "enable DHCP6 server support" ON)
	option(OT_MAC_FILTER "enable mac filter support" ON)
	option(OT_UDP_FORWARD "enable UDP forward support" ON)
	option(OT_SNTP_CLIENT "enable SNTP Client support" ON)
	option(OT_DNS_CLIENT "enable DNS client support" ON)
endif()

option(OT_SLAAC "enable support for adding of auto-configured SLAAC addresses by OpenThread" ON)
option(OT_COAP "enable coap api support" ON)
option(OT_DHCP6_CLIENT "enable DHCP6 client support" ON)

# Pre add-subdirectory config for openthread
set(OPENTHREAD_USE_EXTERN_MAC ON)
list(APPEND OT_PLATFORM_DEFINES
	"OPENTHREAD_CONFIG_EXTERNAL_MAC_SHARED_DD=1"
	"OPENTHREAD_CONFIG_EXTERNAL_MAC_DEVICE_TABLE_SIZE=${CASCODA_DEVICE_TABLE_SIZE}"
	"OPENTHREAD_CONFIG_PLATFORM_INFO=\"cascoda-sdk\""
	"OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE=4096"
	"OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS=${CASCODA_OPENTHREAD_MESSAGE_BUFS}"
	"OPENTHREAD_CONFIG_TCP_ENABLE=0"
	"OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD=8"
)

if(NOT WIN32)
	#Don't use APP logging on windows, because it relies on weak symbols which are broken with mingw gcc!
	list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_APP")
endif()

if(CASCODA_OPENTHREAD_FTD)
	# We do not want to run out of multicast addresses on border routers, as they do not fail gracefully
	# and briefly get stuck in a loop of restarting the Cascoda ot-ncp-posix binary.
	# Fifty thousand entries need 1MB of RAM, which we can spare on our BRs.
	if(UNIX)
		list(APPEND OT_PLATFORM_DEFINES
			"OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=50000"
			"OPENTHREAD_CONFIG_COMMISSIONER_MAX_JOINER_ENTRIES=64"
		)
	else()
		list(APPEND OT_PLATFORM_DEFINES
			"OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=${CASCODA_MAX_EXT_MCAST_ADDR}"
		)
	endif()
endif()

FetchContent_GetProperties(openthread)
if(NOT openthread_POPULATED)
  FetchContent_Populate(openthread)
  add_subdirectory(${openthread_SOURCE_DIR} ${openthread_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Mark any incompatible options as advanced, so they are not presented in CMake UI
mark_as_advanced(FORCE OT_TIME_SYNC OT_LINK_RAW)

add_library(openthread-plat-api INTERFACE)

target_include_directories(openthread-plat-api
	INTERFACE
		${openthread_SOURCE_DIR}/include
		${openthread_SOURCE_DIR}/src/core
		${openthread_BINARY_DIR}/etc/cmake
)

target_compile_definitions(mbedcrypto PUBLIC MBEDTLS_USER_CONFIG_FILE="mbedtls_user_config.h")
target_include_directories(mbedcrypto PUBLIC ${PROJECT_SOURCE_DIR}/mbedtls-user-config)

if(CASCODA_BUILD_SECURE_LWM2M)
	target_compile_definitions(mbedcrypto PUBLIC CASCODA_BUILD_SECURE_LWM2M)
endif()
if(CASCODA_BUILD_LWIP)
	target_compile_definitions(mbedcrypto PUBLIC CASCODA_BUILD_LWIP)
endif()
if(CASCODA_BUILD_KNX)
	target_compile_definitions(mbedcrypto PUBLIC CASCODA_BUILD_KNX)
endif()


target_include_directories(mbedcrypto
	INTERFACE
		${openthread_SOURCE_DIR}/third_party/mbedtls/repo/include
		${openthread_SOURCE_DIR}/third_party/mbedtls
		${openthread_BINARY_DIR}/third_party/mbedtls
)

target_link_libraries(openthread-cli-ftd PUBLIC openthread-ftd-obj)
target_link_libraries(openthread-cli-mtd PUBLIC openthread-mtd-obj)
target_link_libraries(openthread-ncp-ftd PUBLIC openthread-ftd-obj)
target_link_libraries(openthread-ncp-mtd PUBLIC openthread-mtd-obj)

target_link_libraries(mbedcrypto PUBLIC openthread-plat-api)

if(TARGET mbedcrypto-plat)
	target_link_libraries(mbedcrypto PUBLIC mbedcrypto-plat)
endif()

target_compile_definitions(openthread-plat-api
	INTERFACE
		${OT_PLATFORM_DEFINES}
)

# utils ----------------------------------------------------------
add_subdirectory(ca-ot-util)
