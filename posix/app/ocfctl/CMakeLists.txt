project(ocfctl)

add_executable(ocfctl ${PROJECT_SOURCE_DIR}/ocfctl.c)

target_link_libraries(ocfctl ca821x-posix)