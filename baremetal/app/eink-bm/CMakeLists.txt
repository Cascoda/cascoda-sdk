# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-eink-bm)

if(NOT TARGET cascoda-chili2)
	return()
endif()

# Main library config ---------------------------------------------------------
add_executable(eink-example
	${PROJECT_SOURCE_DIR}/source/eink_main.c
	${PROJECT_SOURCE_DIR}/source/eink_image.c
	)

target_include_directories(eink-example
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(eink-example test15-4 cascoda-bm-plat cascoda-bm sensorif)

# convert to bin file format
cascoda_make_binary(eink-example)
