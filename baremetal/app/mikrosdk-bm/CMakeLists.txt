# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-mikrosdk-bm)

# Main library config ---------------------------------------------------------
add_executable(mikrosdk-test
	${PROJECT_SOURCE_DIR}/source/mikrosdk_app.c
	${PROJECT_SOURCE_DIR}/source/mikrosdk_main.c
	)

target_include_directories(mikrosdk-test
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(mikrosdk-test test15-4-api cascoda-bm mikrosdk-click)

# convert to bin file format
cascoda_make_binary(mikrosdk-test CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test mikrosdk-test)