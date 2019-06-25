# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-tempsense)

# Main executable config ------------------------------------------------------
add_executable(tempsense
	${PROJECT_SOURCE_DIR}/source/tempsense_app.c
	${PROJECT_SOURCE_DIR}/source/tempsense_app_coord.c
	${PROJECT_SOURCE_DIR}/source/tempsense_app_device.c
	${PROJECT_SOURCE_DIR}/source/tempsense_debug.c
	${PROJECT_SOURCE_DIR}/source/tempsense_evbme.c
	${PROJECT_SOURCE_DIR}/source/tempsense_main.c
	${PROJECT_SOURCE_DIR}/source/chili_test.c
	)

target_link_libraries(tempsense cascoda-bm test15-4)

target_include_directories( tempsense
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

# convert to bin file format
cascoda_make_binary(tempsense)
