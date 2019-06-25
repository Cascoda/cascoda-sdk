# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-sensorif-bm)

# Main library config ---------------------------------------------------------
add_executable(sensorif-example
	${PROJECT_SOURCE_DIR}/source/sensorif_app.c
	${PROJECT_SOURCE_DIR}/source/sensorif_main.c
	)

target_include_directories(sensorif-example
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif-example sensorif test15-4)

# convert to bin file format
cascoda_make_binary(sensorif-example)
