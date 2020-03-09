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
	set(OT_TAG 022e568a2d1239b4e1d152ef31545ed013830856)
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

# Pre add-subdirectory config for openthread
set(OPENTHREAD_USE_EXTERN_MAC ON)
list(APPEND OT_PLATFORM_DEFINES
	"OPENTHREAD_CONFIG_EXTERNAL_MAC_SHARED_DD=1"
	"OPENTHREAD_CONFIG_EXTERNAL_MAC_DEVICE_TABLE_SIZE=${CASCODA_DEVICE_TABLE_SIZE}"
	"OPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_${CASCODA_LOG_LEVEL}"
	"OPENTHREAD_CONFIG_PLATFORM_INFO=\"cascoda-sdk\""
	"OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE=4096"
	"OPENTHREAD_CONFIG_NCP_UART_ENABLE=1"
	"OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE=1"
	"OPENTHREAD_CONFIG_COAP_API_ENABLE=1"
	"OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE=1"
	"OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS=${CASCODA_OPENTHREAD_MESSAGE_BUFS}"
)

if(NOT WIN32)
	#Don't use NCP_SPINEL logging on windows, because it relies on weak symbols which are broken with mingw gcc!
	list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL")
endif()

if(CASCODA_OPENTHREAD_FTD)
	list(APPEND OT_PLATFORM_DEFINES
		"OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE=1"
		"OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE=1"
		"OPENTHREAD_CONFIG_COMMISSIONER_ENABLE=1"
		"OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE=1"
		"OPENTHREAD_CONFIG_JOINER_ENABLE=1"
		"OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE=1"
		"OPENTHREAD_CONFIG_NCP_UART_ENABLE=1"
		"OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE=1"
		"OPENTHREAD_CONFIG_MAC_FILTER_ENABLE=1"
		"OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE=1"
		"OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=7"
	)
endif()

FetchContent_GetProperties(openthread)
if(NOT openthread_POPULATED)
  FetchContent_Populate(openthread)
  add_subdirectory(${openthread_SOURCE_DIR} ${openthread_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_library(openthread-plat-api INTERFACE)

target_include_directories(openthread-plat-api
	INTERFACE
		${openthread_SOURCE_DIR}/include
		${openthread_SOURCE_DIR}/src/core
		${openthread_BINARY_DIR}/etc/cmake
)

if(${OT_BUILTIN_MBEDTLS})
	target_include_directories(mbedcrypto
		INTERFACE
				${openthread_SOURCE_DIR}/third_party/mbedtls/repo/include
				${openthread_SOURCE_DIR}/third_party/mbedtls
	)
elseif()
	set(MBEDTLS_DEFINITIONS "${MBEDTLS_DEFINITIONS} ${MBEDTLS_INCLUDES}")
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

if(${OT_BUILTIN_MBEDTLS})
	target_compile_definitions(mbedcrypto
		PUBLIC
			MBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"
	)
endif()
