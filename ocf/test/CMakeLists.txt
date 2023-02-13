project(iotivity-tests)

add_executable(ocf-storage-test
    ${PROJECT_SOURCE_DIR}/iotivity_storage_test.c
    )

target_link_libraries(ocf-storage-test ca821x-openthread-bm-ftd iotivity-secure-server)
cascoda_make_binary(ocf-storage-test CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test ocf-storage-test)
