# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-eink-bm)

if(NOT TARGET chili2)
	return()
endif()

# Main library config ---------------------------------------------------------
add_executable(eink-test
	${PROJECT_SOURCE_DIR}/source/eink_main.c
	)
    add_executable(gfx-test
	${PROJECT_SOURCE_DIR}/source/gfx_main.c
	)

target_include_directories(eink-test
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)


target_include_directories(gfx-test
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(eink-test test15-4-api cascoda-bm sensorif eink-image)
target_link_libraries(gfx-test test15-4-api cascoda-bm sensorif)

# convert to bin file format
cascoda_make_binary(eink-test CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test eink-test)


cascoda_make_binary(gfx-test CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test gfx-test)
