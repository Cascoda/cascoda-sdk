# Global config ---------------------------------------------------------------
project (cascoda-dummy-posix)

# Return if not suitable for system processor
if(NOT (UNIX OR MINGW))
	return()
endif()

# Return if platform already configured
if(TARGET cascoda-bm-plat)
	return()
endif()

if(NOT CASCODA_BUILD_DUMMY)
	return()
else()
	message(WARNING "Warning: Configured to build dummy platform instead of Posix SDK - ONLY useful for unit tests!")
endif()

# Mark the device as being FTD-capable
set(CASCODA_OPENTHREAD_FTD ON CACHE INTERNAL "")

# Main library config ---------------------------------------------------------
add_library(cascoda-dummy
	${PROJECT_SOURCE_DIR}/port/source/cascoda_bsp_dummy.c
	${PROJECT_SOURCE_DIR}/port/source/cascoda_time.c
	)

target_link_libraries(cascoda-dummy
	PUBLIC
		cascoda-bm
	)

# Export the chili2 platform as cascoda-bm-plat
add_library(cascoda-bm-plat ALIAS cascoda-dummy)
