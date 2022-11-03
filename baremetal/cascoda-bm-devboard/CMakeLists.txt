# Global config ---------------------------------------------------------------
project (cascoda-bm-devboard)

add_library(cascoda-bm-devboard
    ${PROJECT_SOURCE_DIR}/source/cascoda_devboard_click.c
    ${PROJECT_SOURCE_DIR}/source/cascoda_devboard_btn.c
)


target_link_libraries(cascoda-bm-devboard PUBLIC cascoda-bm mikrosdk-click)

target_include_directories(cascoda-bm-devboard PUBLIC ${PROJECT_SOURCE_DIR}/include)

cascoda_use_warnings(cascoda-bm-devboard)


add_executable(devboard-blinky
    ${PROJECT_SOURCE_DIR}/example/cascoda_devboard_blinky.c
)
target_link_libraries(devboard-blinky cascoda-bm cascoda-bm-devboard)
cascoda_make_binary(devboard-blinky)
cascoda_put_subdir(devboard devboard-blinky)


add_executable(devboard-click
    ${PROJECT_SOURCE_DIR}/example/cascoda_devboard_clickapp.c
)

target_link_libraries(devboard-click cascoda-bm cascoda-bm-devboard mikrosdk-click)
cascoda_make_binary(devboard-click)
cascoda_put_subdir(devboard devboard-click)