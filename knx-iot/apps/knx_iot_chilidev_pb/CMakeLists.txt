add_executable(knx-iot-chilidev-pb
    ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_pb/knx_iot_virtual_pb.c
    ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_pb/knx_iot_chilidev_pb.c
    ${PROJECT_SOURCE_DIR}/knx_iot_wakeful_main.c
)

target_link_libraries(knx-iot-chilidev-pb
    kisClientServer
    kis-port
    ca821x-openthread-bm-mtd
    cascoda-bm-devboard
)

target_include_directories(knx-iot-chilidev-pb
    PRIVATE
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/knx_iot_chilidev_pb/
)

target_compile_definitions(knx-iot-chilidev-pb PRIVATE NO_MAIN)

cascoda_make_binary(knx-iot-chilidev-pb)
cascoda_configure_memory(knx-iot-chilidev-pb 0x1500 0xc000)