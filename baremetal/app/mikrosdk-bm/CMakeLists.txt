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

target_link_libraries(mikrosdk-test test15-4 cascoda-bm)
if(TARGET mikrosdk-click)
	target_link_libraries(mikrosdk-test mikrosdk-click)
endif()

# convert to bin file format
cascoda_make_binary(mikrosdk-test)
cascoda_put_subdir(test mikrosdk-test)