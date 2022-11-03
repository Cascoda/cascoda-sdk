add_executable(knx-iot-chilidev-sa
    ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_sa/knx_iot_virtual_sa.c
    ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_sa/knx_iot_chilidev_sa.c
    ${PROJECT_SOURCE_DIR}/knx_iot_wakeful_main.c
)

target_link_libraries(knx-iot-chilidev-sa
    kisClientServer
    kis-port
    ca821x-openthread-bm-mtd
    cascoda-bm-devboard
)

target_include_directories(knx-iot-chilidev-sa
    PRIVATE
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_sa/
)

target_compile_definitions(knx-iot-chilidev-sa PRIVATE NO_MAIN)

cascoda_make_binary(knx-iot-chilidev-sa)
cascoda_configure_memory(knx-iot-chilidev-sa 0x1500 0xc000)