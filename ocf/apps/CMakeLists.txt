project(iotivity-apps)

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

cascoda_configure_memory(ocf-cli-thermometer 0x1700 0xb000)
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

cascoda_configure_memory(ocf-sleepy-thermometer 0x1700 0xb000)
target_compile_definitions(ocf-sleepy-thermometer PRIVATE)
cascoda_make_binary(ocf-sleepy-thermometer)

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

cascoda_configure_memory(ocf-sensorif 0x1700 0xb000)
target_compile_definitions(ocf-sensorif PRIVATE)
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

cascoda_configure_memory(ocf-sensorif-unsecure 0x1700 0xb000)
target_compile_definitions(ocf-sensorif-unsecure PRIVATE)
cascoda_make_binary(ocf-sensorif-unsecure)

add_executable(ocf-light
    ${PROJECT_SOURCE_DIR}/example_light.c
    ${PROJECT_SOURCE_DIR}/wakeful_main.c
    #${PROJECT_SOURCE_DIR}/wrap_malloc.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-light
    iotivity-secure-server
    ca821x-openthread-bm-mtd
)

target_include_directories(ocf-light BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_light)

target_compile_definitions(ocf-light PRIVATE NO_MAIN)

cascoda_configure_memory(ocf-light 0x1700 0xc400)
cascoda_make_binary(ocf-light)

add_executable(ocf-reed-light
    ${PROJECT_SOURCE_DIR}/example_light.c
    ${PROJECT_SOURCE_DIR}/wakeful_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-reed-light
    iotivity-secure-server
    ca821x-openthread-bm-ftd
)

target_include_directories(ocf-reed-light BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_light)

target_compile_definitions(ocf-reed-light PRIVATE NO_MAIN)

cascoda_configure_memory(ocf-reed-light 0x1700 0xba00)
cascoda_make_binary(ocf-reed-light)

add_executable(ocf-reed-module
    ${PROJECT_SOURCE_DIR}/example_module.c
    ${PROJECT_SOURCE_DIR}/wakeful_main.c
    ${iotivitylite_SOURCE_DIR}/api/oc_introspection.c
)

target_link_libraries(ocf-reed-module
    iotivity-secure-server
    ca821x-openthread-bm-ftd
)

target_include_directories(ocf-reed-module BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include_module)

target_compile_definitions(ocf-reed-module PRIVATE NO_MAIN)

cascoda_configure_memory(ocf-reed-module 0x1700 0xba00)
cascoda_make_binary(ocf-reed-module)
