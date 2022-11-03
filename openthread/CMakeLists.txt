# Global config ---------------------------------------------------------------
project (ca821x-openthread)

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

# Since Iotivity-Lite requires its own patched version of mbedTLS, OpenThread's must be disabled when it is built.
if(${CASCODA_BUILD_OCF})
	set(OT_BUILTIN_MBEDTLS OFF CACHE BOOL "Enable builtin mbedTLS" FORCE)
else()
	set(OT_BUILTIN_MBEDTLS ON CACHE BOOL "Enable builtin mbedTLS" FORCE)
endif()

mark_as_advanced(OT_BUILTIN_MBEDTLS)

# Sub-project configuration ---------------------------------------------------
include(FetchContent)

# Use specific OpenThread commit, unless developing OpenThread
if(CASCODA_OPENTHREAD_DEV)
	set(OT_TAG ext-mac-dev)
else()
	set(OT_TAG 2432fb08e5e2219bc7f816334ef91817a8181140)
endif()

# Only set the openthread repo if it is not already set (user can point to mirror)
if(NOT CASCODA_OPENTHREAD_REPO)
	set(CASCODA_OPENTHREAD_REPO "https://github.com/Cascoda/openthread.git")
endif()

FetchContent_Declare(
  openthread
  GIT_REPOSITORY ${CASCODA_OPENTHREAD_REPO}
  GIT_TAG        ${OT_TAG}
)

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
	"OPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_${CASCODA_LOG_LEVEL}"
	"OPENTHREAD_CONFIG_PLATFORM_INFO=\"cascoda-sdk\""
	"OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE=4096"
	"OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS=${CASCODA_OPENTHREAD_MESSAGE_BUFS}"
)

if(NOT WIN32)
	#Don't use NCP_SPINEL logging on windows, because it relies on weak symbols which are broken with mingw gcc!
	list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL")
endif()

if(CASCODA_OPENTHREAD_FTD)
	# We do not want to run out of multicast addresses on border routers, as they do not fail gracefully
	# and briefly get stuck in a loop of restarting the Cascoda ot-ncp-posix binary.
	# Fifty thousand entries need 1MB of RAM, which we can spare on our BRs.
	if(UNIX)
		list(APPEND OT_PLATFORM_DEFINES
			"OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=50000"
		)
	else()
		list(APPEND OT_PLATFORM_DEFINES
			"OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=7"
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

if(${OT_BUILTIN_MBEDTLS})
	target_include_directories(mbedcrypto
		INTERFACE
			${openthread_SOURCE_DIR}/third_party/mbedtls/repo/include
			${openthread_SOURCE_DIR}/third_party/mbedtls
	)
endif()

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
