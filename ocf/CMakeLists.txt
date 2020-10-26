project (iotivity)

include(FetchContent)

# Cannot build Iotivity without its patched mbedtls
if(NOT ${CASCODA_BUILD_OCF})
    return()
endif()

# Only the Chili 2 has enough memory and flash to run Iotivity
if(NOT TARGET cascoda-chili2)
	message(FATAL_ERROR "OCF is currently only supported for the Chili2 target")
endif()

if(NOT CASCODA_IOTIVITY_REPO)
    set(CASCODA_IOTIVITY_REPO https://github.com/iotivity/iotivity-lite.git)
endif()

# Download the Iotivity Lite source
FetchContent_Declare(
    iotivitylite
    GIT_REPOSITORY ${CASCODA_IOTIVITY_REPO}
    GIT_TAG 6d195d348ef2f0ad4993f7691c65a03b0f334ba5
    PATCH_COMMAND  ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/iotivity-patch.cmake
)

FetchContent_GetProperties(iotivitylite)
if(NOT iotivitylite_POPULATED)
    FetchContent_Populate(iotivitylite)
endif()

# Iotivity Patched mbedTLS

# Do not build anything except for the library
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(ENABLE_TESTING "Build mbed TLS tests." OFF)
# Configurable dynamic memory use
set(OC_DYNAMIC_ALLOCATION_ENABLED ON CACHE BOOL "Enable dynamic memory allocation within the OCF stack and MBedtls.")

# If an mbedtls platform layer is defined, add it to the mbedtls list of libs
if(TARGET mbedtls-plat)
    set(libs ${libs} mbedtls-plat)
endif()

add_subdirectory(${iotivitylite_SOURCE_DIR}/deps/mbedtls ${iotivitylite_BINARY_DIR}/deps/mbedtls)

# mbedtls config depends on OpenThread
add_dependencies(mbedcrypto openthread-plat-api)

target_include_directories(mbedcrypto PUBLIC
    ${PROJECT_SOURCE_DIR}/port
    ${iotivitylite_SOURCE_DIR}
    ${iotivitylite_SOURCE_DIR}/include
    ${iotivitylite_SOURCE_DIR}/deps/mbedtls/include
)

target_compile_definitions(mbedcrypto
    PUBLIC
    MBEDTLS_CONFIG_FILE=\"combined_mbedtls_config.h\"
)

target_link_libraries(mbedcrypto PUBLIC mbedtls)

if(OC_DYNAMIC_ALLOCATION_ENABLED)
    target_compile_definitions(mbedcrypto PUBLIC OC_DYNAMIC_ALLOCATION)
endif()

# Compile Iotivity-Lite

# Add the Iotivity port files, maintained by Cascoda
add_library(iotivity-port INTERFACE)

target_sources(iotivity-port INTERFACE
    ${PROJECT_SOURCE_DIR}/port/abort.c
    ${PROJECT_SOURCE_DIR}/port/clock.c
    ${PROJECT_SOURCE_DIR}/port/ipadapter.c
    ${PROJECT_SOURCE_DIR}/port/random.c
    ${PROJECT_SOURCE_DIR}/port/storage.c
    ${PROJECT_SOURCE_DIR}/port/uart.c
)

target_include_directories(iotivity-port INTERFACE
    ${PROJECT_SOURCE_DIR}/port

    ${iotivitylite_SOURCE_DIR}/
    ${iotivitylite_SOURCE_DIR}/include
)

target_link_libraries(iotivity-port INTERFACE openthread-plat-api tinycbor-master)

# Iotivity API
add_library(iotivity-api INTERFACE)

target_sources(iotivity-api INTERFACE
    ${iotivitylite_SOURCE_DIR}/api/oc_base64.c
    ${iotivitylite_SOURCE_DIR}/api/oc_blockwise.c
    ${iotivitylite_SOURCE_DIR}/api/oc_buffer.c
    ${iotivitylite_SOURCE_DIR}/api/oc_client_api.c
    ${iotivitylite_SOURCE_DIR}/api/oc_clock.c
    ${iotivitylite_SOURCE_DIR}/api/oc_collection.c
    ${iotivitylite_SOURCE_DIR}/api/oc_core_res.c
    ${iotivitylite_SOURCE_DIR}/api/oc_discovery.c
    ${iotivitylite_SOURCE_DIR}/api/oc_endpoint.c
    ${iotivitylite_SOURCE_DIR}/api/oc_helpers.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
    ${iotivitylite_SOURCE_DIR}/api/oc_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_mnt.c
    ${iotivitylite_SOURCE_DIR}/api/oc_network_events.c
    ${iotivitylite_SOURCE_DIR}/api/oc_rep.c
    ${iotivitylite_SOURCE_DIR}/api/oc_resource_factory.c
    ${iotivitylite_SOURCE_DIR}/api/oc_ri.c
    ${iotivitylite_SOURCE_DIR}/api/oc_server_api.c
    ${iotivitylite_SOURCE_DIR}/api/oc_session_events.c
    ${iotivitylite_SOURCE_DIR}/api/oc_swupdate.c
    ${iotivitylite_SOURCE_DIR}/api/oc_uuid.c
)

target_include_directories(iotivity-api INTERFACE
    ${iotivitylite_SOURCE_DIR}/
    ${iotivitylite_SOURCE_DIR}/api
    ${iotivitylite_SOURCE_DIR}/include
)

# Core functions used by Iotivity
add_library(iotivity-common INTERFACE)

target_sources(iotivity-common INTERFACE
    # Utilities that are used deep within Iotivity
    ${iotivitylite_SOURCE_DIR}/util/oc_etimer.c
    ${iotivitylite_SOURCE_DIR}/util/oc_list.c
    ${iotivitylite_SOURCE_DIR}/util/oc_memb.c
    ${iotivitylite_SOURCE_DIR}/util/oc_mem_trace.c
    ${iotivitylite_SOURCE_DIR}/util/oc_mmem.c
    ${iotivitylite_SOURCE_DIR}/util/oc_process.c
    ${iotivitylite_SOURCE_DIR}/util/oc_timer.c
    # Security
    ${iotivitylite_SOURCE_DIR}/security/oc_acl.c
    ${iotivitylite_SOURCE_DIR}/security/oc_certs.c
    ${iotivitylite_SOURCE_DIR}/security/oc_cred.c
    ${iotivitylite_SOURCE_DIR}/security/oc_csr.c
    ${iotivitylite_SOURCE_DIR}/security/oc_doxm.c
    ${iotivitylite_SOURCE_DIR}/security/oc_keypair.c
    ${iotivitylite_SOURCE_DIR}/security/oc_obt.c
    ${iotivitylite_SOURCE_DIR}/security/oc_obt_certs.c
    ${iotivitylite_SOURCE_DIR}/security/oc_obt_otm_cert.c
    ${iotivitylite_SOURCE_DIR}/security/oc_obt_otm_justworks.c
    ${iotivitylite_SOURCE_DIR}/security/oc_obt_otm_randompin.c
    ${iotivitylite_SOURCE_DIR}/security/oc_pki.c
    ${iotivitylite_SOURCE_DIR}/security/oc_pstat.c
    ${iotivitylite_SOURCE_DIR}/security/oc_roles.c
    ${iotivitylite_SOURCE_DIR}/security/oc_sp.c
    ${iotivitylite_SOURCE_DIR}/security/oc_store.c
    ${iotivitylite_SOURCE_DIR}/security/oc_svr.c
    ${iotivitylite_SOURCE_DIR}/security/oc_tls.c

)

target_include_directories(iotivity-common INTERFACE
    ${iotivitylite_SOURCE_DIR}/
    ${iotivitylite_SOURCE_DIR}/include
    ${iotivitylite_SOURCE_DIR}/deps/tinycbor/src
    ${iotivitylite_SOURCE_DIR}/security
)

target_link_libraries(iotivity-common INTERFACE iotivity-port mbedtls tinycbor-master)

# Iotivity's implementation of CoAP
add_library(iotivity-coap INTERFACE)

target_sources(iotivity-coap INTERFACE
    ${iotivitylite_SOURCE_DIR}/messaging/coap/coap.c
    ${iotivitylite_SOURCE_DIR}/messaging/coap/coap_signal.c
    ${iotivitylite_SOURCE_DIR}/messaging/coap/engine.c
    ${iotivitylite_SOURCE_DIR}/messaging/coap/observe.c
    ${iotivitylite_SOURCE_DIR}/messaging/coap/separate.c
    ${iotivitylite_SOURCE_DIR}/messaging/coap/transactions.c
)

target_include_directories(iotivity-coap INTERFACE
    ${iotivitylite_SOURCE_DIR}/messaging/coap/
)

target_link_libraries(iotivity-coap INTERFACE iotivity-port)

# Client and server versions of Iotivity
add_library(iotivity-client INTERFACE)

target_link_libraries(iotivity-client INTERFACE
    iotivity-coap
    iotivity-common
    iotivity-api
)

target_compile_definitions(iotivity-client INTERFACE OC_CLIENT)

add_library(iotivity-server INTERFACE)

target_link_libraries(iotivity-server INTERFACE
    iotivity-coap
    iotivity-common
    iotivity-api
)

target_compile_definitions(iotivity-server INTERFACE OC_SERVER)

# Configurable debugging
set(OC_DEBUG_ENABLED OFF CACHE BOOL "Enable debug messages from the Iotivity stack.")

if(OC_DEBUG_ENABLED)
    target_compile_definitions(iotivity-server INTERFACE OC_DEBUG)
    target_compile_definitions(iotivity-client INTERFACE OC_DEBUG)
    target_compile_definitions(iotivity-common INTERFACE OC_DEBUG)
endif()

# Add static libraries for various configurations of Iotivity-Lite

add_library(iotivity-unsecure-server)
target_link_libraries(iotivity-unsecure-server PRIVATE iotivity-server)
target_link_libraries(iotivity-unsecure-server PUBLIC ca821x-openthread-bm-plat)
target_compile_definitions(iotivity-unsecure-server PUBLIC OC_SERVER)

add_library(iotivity-secure-server)
target_link_libraries(iotivity-secure-server PRIVATE iotivity-server)
target_link_libraries(iotivity-secure-server PUBLIC ca821x-openthread-bm-plat)
target_compile_definitions(iotivity-secure-server PUBLIC OC_SERVER OC_SECURITY)

add_library(iotivity-pki-server)
target_link_libraries(iotivity-pki-server PRIVATE iotivity-server)
target_link_libraries(iotivity-pki-server PUBLIC ca821x-openthread-bm-plat)
target_compile_definitions(iotivity-pki-server PUBLIC OC_SERVER OC_SECURITY OC_PKI)

add_library(iotivity-pki-client)
target_link_libraries(iotivity-pki-client PRIVATE iotivity-client)
target_link_libraries(iotivity-pki-client PUBLIC ca821x-openthread-bm-plat)
target_compile_definitions(iotivity-pki-client PUBLIC OC_CLIENT OC_SECURITY OC_PKI)

# Build instructions for Iotivity-Lite demo apps can be found here
add_subdirectory(${PROJECT_SOURCE_DIR}/apps)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)