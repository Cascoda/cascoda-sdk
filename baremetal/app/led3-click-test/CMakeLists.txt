# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-led3-click)

# Main library config ---------------------------------------------------------
add_library(led3
	${PROJECT_SOURCE_DIR}/source/led3_click_test.c
	${PROJECT_SOURCE_DIR}/source/led3_click_main.c
	)

target_include_directories(led3
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(led3
	PUBLIC
		cascoda-bm
		mikrosdk-click
	)

# Test app config -------------------------------------------------------------
add_executable(led3-click ${PROJECT_SOURCE_DIR}/source/led3_click_main.c)
target_link_libraries(led3-click led3 cascoda-bm)

# convert to bin file format
cascoda_make_binary(led3-click CASCODA_BUILD_BINARIES)