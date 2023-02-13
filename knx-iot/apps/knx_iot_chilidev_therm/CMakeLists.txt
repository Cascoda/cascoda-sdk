add_executable(knx-iot-chilidev-therm
    ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_therm/room_temperature_sensor.c
    ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_therm/knx_iot_chilidev_therm.c
    ${PROJECT_SOURCE_DIR}/knx_iot_wakeful_main.c
)

target_link_libraries(knx-iot-chilidev-therm
    kisClientServer
    kis-port
    ca821x-openthread-bm-mtd
    cascoda-bm-devboard
)

target_include_directories(knx-iot-chilidev-therm
    PRIVATE
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_therm/
)

target_compile_definitions(knx-iot-chilidev-therm PRIVATE NO_MAIN)

cascoda_make_binary(knx-iot-chilidev-therm CASCODA_BUILD_BINARIES)
cascoda_configure_memory(knx-iot-chilidev-therm 0x1500 0xbc00)
