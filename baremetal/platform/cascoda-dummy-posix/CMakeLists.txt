# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.12)
project (cascoda-dummy-posix)

# Return if not suitable for system processor
if(NOT (UNIX OR MINGW))
	return()
endif()

# Return if platform already configured
if(TARGET cascoda-bm-plat)
	return()
endif()

# define the 'interface' library that makes up the bm platform.
add_library(cascoda-bm-plat INTERFACE)


# Main library config ---------------------------------------------------------
add_library(cascoda-dummy
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_dummy.c
	)

target_link_libraries(cascoda-dummy
	PUBLIC
		cascoda-bm
	)

target_include_directories(cascoda-dummy
	PUBLIC
		${PROJECT_SOURCE_DIR}/port/include
		${PROJECT_BINARY_DIR}/port/include
	)

# Export the chili2 platform as cascoda-bm-plat
target_link_libraries(cascoda-bm-plat INTERFACE cascoda-dummy)
