# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (cascoda-test15-4-bm)

# Main library config ---------------------------------------------------------
add_library(test15-4
	${PROJECT_SOURCE_DIR}/source/test15_4_evbme.c
	)

target_include_directories(test15-4
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

# link to the platform library that should be
# provided by the superproject and also the
# test15-4-api provided by the ca821x-api
target_link_libraries(test15-4
	PUBLIC
		cascoda-bm
		test15-4-api
	)

# Test app config -------------------------------------------------------------
add_executable(test15-4-app ${PROJECT_SOURCE_DIR}/source/test15_4_main.c)
target_link_libraries(test15-4-app test15-4)

# convert to bin file format
cascoda_make_binary(test15-4-app)
