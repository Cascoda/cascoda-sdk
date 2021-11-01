# Global config ---------------------------------------------------------------
project (cascoda-util)

# Main library config ---------------------------------------------------------
add_library(cascoda-util
	${PROJECT_SOURCE_DIR}/src/cascoda_hash.c
	${PROJECT_SOURCE_DIR}/src/cascoda_rand.c
	${PROJECT_SOURCE_DIR}/src/cascoda_tasklet.c
	${PROJECT_SOURCE_DIR}/src/cascoda_time.c
	${PROJECT_SOURCE_DIR}/src/cascoda_settings.c
	)

target_include_directories(cascoda-util
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
	
target_link_libraries(cascoda-util
	PUBLIC
		ca821x-api
	)

cascoda_use_warnings(cascoda-util)

# Tests
add_subdirectory(test)
