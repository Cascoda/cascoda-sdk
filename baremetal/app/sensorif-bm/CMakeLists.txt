# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-sensorif-bm)

if (NOT CMAKE_HOST_UNIX)
	message(WARNING "Building Openthread on windows is a challenge. Contributions welcome but it's probably easier to switch to Linux.")
	message(WARNING "Skipping openthread build due to non-unix build system!")
	return()
endif()

# Main library config ---------------------------------------------------------
add_executable(sensorif-example
	${PROJECT_SOURCE_DIR}/source/sensorif_app.c
	${PROJECT_SOURCE_DIR}/source/sensorif_main.c
	)

target_include_directories(sensorif-example
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif-example sensorif test15-4 cascoda-bm cascoda-bm-plat)

# convert to bin file format
cascoda_make_binary(sensorif-example)
