project (iotivity)

include(FetchContent)


# Cannot build Iotivity without its patched mbedtls
if(NOT ${CASCODA_BUILD_OCF})
    return()
endif()

option(CASCODA_BUILD_OCF_PKI "Enable PKI security in all secure OCF targets" ON)

if(${CASCODA_BUILD_OCF_PKI})
    # Download PKI Test certs
    find_package (Python3)
    message("Python3_Interpreter_FOUND = " ${Python3_Interpreter_FOUND})
    
    get_target_property(PYTHON3 Python3::Interpreter LOCATION)

    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/pki_certs)

    execute_process(COMMAND ${PYTHON3} ${PROJECT_SOURCE_DIR}/pki_certs_update.py
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/pki_certs
        RESULT_VARIABLE PYTHON_RESULT
    )

    if(PYTHON_RESULT)
        message(FATAL_ERROR "Failed to download PKI Certs")
    endif()
endif()

# Only the Chili 2 has enough memory and flash to run Iotivity
if(NOT TARGET cascoda-chili2)
	message(FATAL_ERROR "OCF is currently only supported for the Chili2 target")
endif()

if(NOT CASCODA_IOTIVITY_REPO)
    set(CASCODA_IOTIVITY_REPO https://github.com/Cascoda/iotivity-lite.git)
endif()

# Download the Iotivity Lite source
FetchContent_Declare(
    iotivitylite
    GIT_REPOSITORY ${CASCODA_IOTIVITY_REPO}
    GIT_TAG 9105d5cfdf732c414c2211b41d40d55acdd63b3a
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
if(TARGET mbedcrypto-plat)
	target_link_libraries(mbedcrypto PUBLIC mbedcrypto-plat)
endif()

if(OC_DYNAMIC_ALLOCATION_ENABLED)
    target_compile_definitions(mbedcrypto PUBLIC OC_DYNAMIC_ALLOCATION)
endif()

add_subdirectory(port)

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
    ${iotivitylite_SOURCE_DIR}/api/oc_enums.c
    ${iotivitylite_SOURCE_DIR}/api/oc_helpers.c
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
    ${iotivitylite_SOURCE_DIR}/api/c-timestamp/timestamp_compare.c
    ${iotivitylite_SOURCE_DIR}/api/c-timestamp/timestamp_format.c
    ${iotivitylite_SOURCE_DIR}/api/c-timestamp/timestamp_parse.c
    ${iotivitylite_SOURCE_DIR}/api/c-timestamp/timestamp_tm.c
    ${iotivitylite_SOURCE_DIR}/api/c-timestamp/timestamp_valid.c
)

target_include_directories(iotivity-api INTERFACE
    ${iotivitylite_SOURCE_DIR}/
    ${iotivitylite_SOURCE_DIR}/api
    ${iotivitylite_SOURCE_DIR}/api/c-timestamp
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
    ${iotivitylite_SOURCE_DIR}/security/oc_ael.c
    ${iotivitylite_SOURCE_DIR}/security/oc_audit.c
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
    ${iotivitylite_SOURCE_DIR}/security/oc_sdi.c
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

target_link_libraries(iotivity-common INTERFACE mbedtls tinycbor-master)

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
target_link_libraries(iotivity-unsecure-server PUBLIC ca821x-openthread-bm-plat cascoda-util iotivity-port)
target_compile_definitions(iotivity-unsecure-server PUBLIC OC_SERVER)

add_library(iotivity-secure-server)
target_include_directories(iotivity-secure-server PUBLIC ${PROJECT_BINARY_DIR}/pki_certs)
target_link_libraries(iotivity-secure-server PRIVATE iotivity-server)
target_link_libraries(iotivity-secure-server PUBLIC ca821x-openthread-bm-plat cascoda-util iotivity-port)
target_compile_definitions(iotivity-secure-server PUBLIC OC_SERVER OC_SECURITY)

if(CASCODA_BUILD_OCF_PKI)
    target_compile_definitions(mbedcrypto PUBLIC OC_PKI)
endif()

# Build instructions for Iotivity-Lite demo apps can be found here
add_subdirectory(${PROJECT_SOURCE_DIR}/apps)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
