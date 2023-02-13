# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-mac-tempsense)

# Main executable config ------------------------------------------------------
add_executable(mac-tempsense
	${PROJECT_SOURCE_DIR}/source/tempsense_app.c
	${PROJECT_SOURCE_DIR}/source/tempsense_app_coord.c
	${PROJECT_SOURCE_DIR}/source/tempsense_app_device.c
	${PROJECT_SOURCE_DIR}/source/tempsense_debug.c
	${PROJECT_SOURCE_DIR}/source/tempsense_evbme.c
	${PROJECT_SOURCE_DIR}/source/tempsense_main.c
	)

target_link_libraries(mac-tempsense cascoda-bm test15-4-api)

target_include_directories( mac-tempsense
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

# convert to bin file format
cascoda_make_binary(mac-tempsense CASCODA_BUILD_BINARIES)
