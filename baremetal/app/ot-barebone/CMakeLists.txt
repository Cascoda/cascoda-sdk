# Global config ---------------------------------------------------------------
project (ca821x-thread-dev)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "NANO120")
	return()
endif()

# Test app config -------------------------------------------------------------
add_executable(ot-barebone-mtd
	${PROJECT_SOURCE_DIR}/source/thread_dev_main.c
	${PROJECT_SOURCE_DIR}/source/thread_dev_api.c
	)
target_include_directories(ot-barebone-mtd
	PRIVATE ${PROJECT_SOURCE_DIR}/include
	)
target_link_libraries(ot-barebone-mtd ca821x-openthread-bm-mtd)
cascoda_make_binary(ot-barebone-mtd CASCODA_BUILD_BINARIES)

if(CASCODA_OPENTHREAD_FTD)
	add_executable(ot-barebone-ftd
		${PROJECT_SOURCE_DIR}/source/thread_dev_main.c
		${PROJECT_SOURCE_DIR}/source/thread_dev_api.c
		)
	target_include_directories(ot-barebone-ftd
		PRIVATE ${PROJECT_SOURCE_DIR}/include
		)
	target_link_libraries(ot-barebone-ftd ca821x-openthread-bm-ftd)
	cascoda_make_binary(ot-barebone-ftd CASCODA_BUILD_BINARIES)
endif()
