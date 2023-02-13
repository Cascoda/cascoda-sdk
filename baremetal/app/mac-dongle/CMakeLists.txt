# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-mac-dongle)

# Main executable config ------------------------------------------------------
add_executable(mac-dongle
	${PROJECT_SOURCE_DIR}/source/mac_dongle_main.c
	)

target_link_libraries(mac-dongle cascoda-bm test15-4-api)

target_include_directories( mac-dongle
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

# convert to bin file format
cascoda_make_binary(mac-dongle CASCODA_BUILD_BINARIES)
