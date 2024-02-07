# Global config ---------------------------------------------------------------
project (cascoda-bm-devboard)

add_library(cascoda-bm-devboard
    ${PROJECT_SOURCE_DIR}/source/devboard_btn.c
    ${PROJECT_SOURCE_DIR}/source/devboard_batt.c
    ${PROJECT_SOURCE_DIR}/source/devboard_click.c
    ${PROJECT_SOURCE_DIR}/source/devboard_click_handlers_default.c
    ${PROJECT_SOURCE_DIR}/source/knx_click_handlers.c
    ${PROJECT_SOURCE_DIR}/source/devboard_sensorif.c
)


target_link_libraries(cascoda-bm-devboard PUBLIC cascoda-bm cascoda-btn mikrosdk-click)

target_include_directories(cascoda-bm-devboard PUBLIC ${PROJECT_SOURCE_DIR}/include)

cascoda_use_warnings(cascoda-bm-devboard)


add_executable(devboard-btn
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_btn.c
)

target_link_libraries(devboard-btn cascoda-bm cascoda-bm-devboard)
cascoda_make_binary(devboard-btn CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-btn)

add_executable(devboard-click
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_click.c
)

target_link_libraries(devboard-click cascoda-bm cascoda-bm-devboard mikrosdk-click btn-ext-pi4ioe5v96248)
cascoda_make_binary(devboard-click CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-click)

add_executable(devboard-sleep
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_sleep.c
)

target_link_libraries(devboard-sleep cascoda-bm cascoda-bm-devboard mikrosdk-click)
cascoda_make_binary(devboard-sleep CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-sleep)

add_executable(devboard-eink
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_eink.c
)

target_link_libraries(devboard-eink cascoda-bm test15-4-api cascoda-bm-devboard mikrosdk-click sensorif eink-driver-2-9 eink-image-2-9)

cascoda_make_binary(devboard-eink CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-eink)

add_executable(devboard-batt
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_batt.c
)

target_link_libraries(devboard-batt cascoda-bm cascoda-bm-devboard)
cascoda_make_binary(devboard-batt CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-batt)

add_executable(devboard-gfx-2-9
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_gfx_2_9.c
)

target_link_libraries(devboard-gfx-2-9 cascoda-bm test15-4-api cascoda-bm-devboard mikrosdk-click sensorif eink-driver-2-9)
cascoda_make_binary(devboard-gfx-2-9 CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-gfx-2-9)

add_executable(devboard-gfx-waveshare-1-54
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_gfx_1_54.c
)
target_link_libraries(devboard-gfx-waveshare-1-54 cascoda-bm test15-4-api cascoda-bm-devboard mikrosdk-click sensorif eink-driver-waveshare-1-54-half-res) 
cascoda_make_binary(devboard-gfx-waveshare-1-54 CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-gfx-waveshare-1-54)

add_executable(devboard-gfx-mikroe-1-54
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_gfx_1_54.c
)
target_link_libraries(devboard-gfx-mikroe-1-54 cascoda-bm test15-4-api cascoda-bm-devboard mikrosdk-click sensorif eink-driver-mikroe-1-54-half-res) 
cascoda_make_binary(devboard-gfx-mikroe-1-54 CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-gfx-mikroe-1-54)

add_executable(devboard-click-batt
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_click_batt.c
)

target_link_libraries(devboard-click-batt cascoda-bm cascoda-bm-devboard mikrosdk-click)
cascoda_make_binary(devboard-click-batt CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-click-batt)
