# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-sensorif-bm)

# Main library config ---------------------------------------------------------
add_executable(sensorif-test
	${PROJECT_SOURCE_DIR}/source/sensorif_app.c
	${PROJECT_SOURCE_DIR}/source/sensorif_main.c
	)

target_include_directories(sensorif-test
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif-test sensorif test15-4 cascoda-bm)


# convert to bin file format
cascoda_make_binary(sensorif-test)
cascoda_put_subdir(test sensorif-test)
