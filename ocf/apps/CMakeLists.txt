project(iotivity-apps)

option(CASCODA_BUILD_OCF_DEV "Build in-development OCF targets. These are
    unsuitable for production and may be incompatible with certain OpenThread
    configurations" OFF)

mark_as_advanced(CASCODA_BUILD_OCF_DEV)

add_executable(ocf-cli-light
    ${PROJECT_SOURCE_DIR}/example_light.c
    ${PROJECT_SOURCE_DIR}/wakeful_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-cli-light
    openthread-cli-ftd
    iotivity-secure-server
    ca821x-openthread-bm-ftd
)

target_include_directories(ocf-cli-light BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_light)

target_compile_definitions(ocf-cli-light PRIVATE NO_MAIN CASCODA_MAX_APP_DATA_SIZE=8192)
cascoda_configure_memory(ocf-cli-light 0x2600 0x5000)
cascoda_make_binary(ocf-cli-light)

add_executable(ocf-cli-thermometer
    ${PROJECT_SOURCE_DIR}/example_thermometer.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-cli-thermometer
    openthread-cli-ftd
    iotivity-secure-server
    ca821x-openthread-bm-ftd
)

target_include_directories(ocf-cli-thermometer BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_thermometer)

cascoda_configure_memory(ocf-cli-thermometer 0x2600 0x5000)
cascoda_make_binary(ocf-cli-thermometer)

add_executable(ocf-sleepy-thermometer
    ${PROJECT_SOURCE_DIR}/example_sleepy_thermometer.c
    ${PROJECT_SOURCE_DIR}/sleepy_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-sleepy-thermometer
    iotivity-secure-server
    ca821x-openthread-bm-ftd
)

target_include_directories(ocf-sleepy-thermometer BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_thermometer)

cascoda_configure_memory(ocf-sleepy-thermometer 0x2600 0x5000)
target_compile_definitions(ocf-sleepy-thermometer PRIVATE CASCODA_MAX_APP_DATA_SIZE=8192)
cascoda_make_binary(ocf-sleepy-thermometer)


add_executable(ocf-sleepy-thermometer-unsecure
    ${PROJECT_SOURCE_DIR}/example_sleepy_thermometer.c
    ${PROJECT_SOURCE_DIR}/sleepy_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-sleepy-thermometer-unsecure
    iotivity-unsecure-server
    ca821x-openthread-bm-mtd
)
target_include_directories(ocf-sleepy-thermometer-unsecure BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_thermometer)

cascoda_configure_memory(ocf-sleepy-thermometer-unsecure 0x2600 0x7200)
target_compile_definitions(ocf-sleepy-thermometer-unsecure PRIVATE CASCODA_MAX_APP_DATA_SIZE=8192)
cascoda_make_binary(ocf-sleepy-thermometer-unsecure)

add_executable(ocf-sensorif
    ${PROJECT_SOURCE_DIR}/example_sensorif.c
    ${PROJECT_SOURCE_DIR}/sleepy_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-sensorif
    iotivity-secure-server
    ca821x-openthread-bm-ftd
    sensorif
)

target_include_directories(ocf-sensorif BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_sensorif)

cascoda_configure_memory(ocf-sensorif 0x2600 0x5000)
target_compile_definitions(ocf-sensorif PRIVATE CASCODA_MAX_APP_DATA_SIZE=8192)
cascoda_make_binary(ocf-sensorif)

add_executable(ocf-sensorif-unsecure
    ${PROJECT_SOURCE_DIR}/example_sensorif.c
    ${PROJECT_SOURCE_DIR}/sleepy_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-sensorif-unsecure
    iotivity-unsecure-server
    ca821x-openthread-bm-ftd
    sensorif
)

target_include_directories(ocf-sensorif-unsecure BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_sensorif)

cascoda_configure_memory(ocf-sensorif-unsecure 0x2600 0x7200)
target_compile_definitions(ocf-sensorif-unsecure PRIVATE CASCODA_MAX_APP_DATA_SIZE=8192)
cascoda_make_binary(ocf-sensorif-unsecure)

if(CASCODA_BUILD_OCF_DEV) 

    add_executable(ocf-cli-pki-light

        ${PROJECT_SOURCE_DIR}/example_light.c
        ${PROJECT_SOURCE_DIR}/wakeful_main.c
        #${PROJECT_SOURCE_DIR}/wrap_malloc.c

        ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
    )

    target_link_libraries(ocf-cli-pki-light
        openthread-cli-mtd
        iotivity-pki-server
        ca821x-openthread-bm-mtd
    )

    target_include_directories(ocf-cli-pki-light BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_light)

    target_compile_definitions(ocf-cli-pki-light PRIVATE NO_MAIN CASCODA_MAX_APP_DATA_SIZE=8192)

    cascoda_configure_memory(ocf-cli-pki-light 0x2600 0x8000)
    cascoda_make_binary(ocf-cli-pki-light)
endif()
