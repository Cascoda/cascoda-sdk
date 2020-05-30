# Global config ---------------------------------------------------------------
project (cascoda-util-tests)

if(NOT BUILD_TESTING)
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

cascoda_put_subdir(tasklet_test test)
