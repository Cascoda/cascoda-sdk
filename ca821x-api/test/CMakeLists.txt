# Global config ---------------------------------------------------------------
project (cascoda-bm-tests)

if(NOT BUILD_TESTING)
	return()
endif()
# Currently these tests only run on unix
if(NOT (UNIX OR MINGW))
	return()
endif()

# Build non-cmocka test
add_executable(ca821x-api-test
	${PROJECT_SOURCE_DIR}/test.c
	)

target_link_libraries(ca821x-api-test ca821x-api)

# Add tests -------------------------------------------------------------------
add_test(NAME ca821x-api-test COMMAND ca821x-api-test)

add_cmocka_test(helper_test
	SOURCES
		${PROJECT_SOURCE_DIR}/helper_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		ca821x-api
	)

add_cmocka_test(endian_test
	SOURCES
		${PROJECT_SOURCE_DIR}/endian_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		ca821x-api
	)

add_cmocka_test(blacklist_test
	SOURCES
		${PROJECT_SOURCE_DIR}/blacklist_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		ca821x-api
	)

cascoda_put_subdir(ca821x-api-test test)
cascoda_put_subdir(helper_test test)
cascoda_put_subdir(endian_test test)