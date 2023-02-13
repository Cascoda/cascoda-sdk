# Global config ---------------------------------------------------------------
project (cascoda-bm-devboard)

add_library(cascoda-bm-devboard
    ${PROJECT_SOURCE_DIR}/source/devboard_btn.c
    ${PROJECT_SOURCE_DIR}/source/devboard_click.c
    ${PROJECT_SOURCE_DIR}/source/devboard_click_handlers_default.c
    ${PROJECT_SOURCE_DIR}/source/knx_click_handlers.c
    ${PROJECT_SOURCE_DIR}/source/devboard_sensorif.c
)


target_link_libraries(cascoda-bm-devboard PUBLIC cascoda-bm mikrosdk-click)

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

target_link_libraries(devboard-click cascoda-bm cascoda-bm-devboard mikrosdk-click)
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

target_link_libraries(devboard-eink cascoda-bm test15-4-api cascoda-bm-devboard mikrosdk-click sensorif eink-image)

cascoda_make_binary(devboard-eink CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-eink)

add_executable(devboard-gfx
    ${PROJECT_SOURCE_DIR}/examples/devboard_app_gfx.c
)
target_link_libraries(devboard-gfx cascoda-bm test15-4-api cascoda-bm-devboard mikrosdk-click sensorif eink-image)
cascoda_make_binary(devboard-gfx CASCODA_BUILD_BINARIES)
cascoda_put_subdir(devboard devboard-gfx)
