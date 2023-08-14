# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-buzz2-click)

# Main library config ---------------------------------------------------------
add_library(buzz2
	${PROJECT_SOURCE_DIR}/source/buzz2_click_test.c
	${PROJECT_SOURCE_DIR}/source/buzz2_click_main.c
	)

target_include_directories(buzz2
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(buzz2
	PUBLIC
		cascoda-bm
		mikrosdk-click
	)

# Test app config -------------------------------------------------------------
add_executable(buzz2-click ${PROJECT_SOURCE_DIR}/source/buzz2_click_main.c)
target_link_libraries(buzz2-click buzz2 cascoda-bm)

# convert to bin file format
cascoda_make_binary(buzz2-click CASCODA_BUILD_BINARIES)