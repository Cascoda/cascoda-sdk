add_subdirectory(longtest_test)

if(NOT CA_BUILD_TESTING)
    return()
endif()

# Add unit tests -------------------------------------------------------------------
add_cmocka_test(version_test
        SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/version_test.c
        LINK_LIBRARIES
            ${CMOCKA_SHARED_LIBRARY}
            ca821x-posix
        )

cascoda_put_subdir(test version_test)


add_cmocka_test(settings_test
        SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/settings_test.c
        LINK_LIBRARIES
            ${CMOCKA_SHARED_LIBRARY}
            ca821x-posix
        )

cascoda_put_subdir(test settings_test)
