# Global config ---------------------------------------------------------------
project (cascoda-util-tests)

if(NOT CA_BUILD_TESTING)
	return()
endif()

if(NOT (UNIX OR MINGW))
	return()
endif()

# Add tests -------------------------------------------------------------------
add_cmocka_test(tasklet_test
	SOURCES
		${PROJECT_SOURCE_DIR}/tasklet_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		cascoda-util
	)

add_cmocka_test(util_time_test
	SOURCES
	${PROJECT_SOURCE_DIR}/util_time_test.c
	LINK_LIBRARIES
	${CMOCKA_SHARED_LIBRARY}
	cascoda-util
	)
	
add_cmocka_test(hash_test
	SOURCES
	${PROJECT_SOURCE_DIR}/hash_test.c
	LINK_LIBRARIES
	${CMOCKA_SHARED_LIBRARY}
	cascoda-util
	)

cascoda_put_subdir(test tasklet_test util_time_test hash_test)
