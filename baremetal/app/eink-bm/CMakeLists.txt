# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-eink-bm)

if(NOT TARGET chili2)
	return()
endif()

# Main library config ---------------------------------------------------------
add_executable(eink-test-2-9
	${PROJECT_SOURCE_DIR}/source/eink_2_9_main.c
	)
add_executable(eink-test-1-54
	${PROJECT_SOURCE_DIR}/source/eink_1_54_main.c
	)

add_executable(gfx-test
	${PROJECT_SOURCE_DIR}/source/gfx_main.c
	)

target_include_directories(eink-test-2-9
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-test-1-54
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)


target_include_directories(gfx-test
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(eink-test-2-9 test15-4-api cascoda-bm sensorif eink-driver-2-9 eink-image-2-9)
target_link_libraries(eink-test-1-54 test15-4-api cascoda-bm sensorif eink-driver-1-54-full-res eink-image-1-54)
target_link_libraries(gfx-test test15-4-api cascoda-bm sensorif eink-driver-2-9)

# convert to bin file format
cascoda_make_binary(eink-test-2-9 CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test eink-test-2-9)

cascoda_make_binary(eink-test-1-54 CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test eink-test-1-54)

cascoda_make_binary(gfx-test CASCODA_BUILD_BINARIES)
cascoda_put_subdir(test gfx-test)
