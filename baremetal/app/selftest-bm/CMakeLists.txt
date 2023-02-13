# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-selftest-bm)

# Main library config ---------------------------------------------------------
add_executable(self-test
	${PROJECT_SOURCE_DIR}/source/selftest_app.c
	${PROJECT_SOURCE_DIR}/source/selftest_main.c
	)

target_include_directories(self-test
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(self-test cascoda-bm)

# convert to bin file format
cascoda_make_binary(self-test CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test self-test)
