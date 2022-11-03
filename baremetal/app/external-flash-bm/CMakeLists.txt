# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-flash-bm)

if(NOT TARGET cascoda-chili2)
	return()
endif()

# Main library config ---------------------------------------------------------
add_executable(external-flash-test
	${PROJECT_SOURCE_DIR}/flash_test_main.c
	)

target_link_libraries(external-flash-test test15-4-api cascoda-bm)

# convert to bin file format
cascoda_make_binary(external-flash-test)
