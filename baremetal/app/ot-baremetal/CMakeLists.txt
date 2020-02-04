# Global config ---------------------------------------------------------------
project (ca821x-thread-dev)

# Test app config -------------------------------------------------------------
add_executable(mtdapp
	${PROJECT_SOURCE_DIR}/source/thread_dev_main.c
	${PROJECT_SOURCE_DIR}/source/thread_dev_api.c
	)
target_include_directories(mtdapp
	PRIVATE ${PROJECT_SOURCE_DIR}/include
	)
target_link_libraries(mtdapp ca821x-openthread-bm-mtd cascoda-bm-plat)
target_compile_definitions(mtdapp PRIVATE APP_NAME="OT MTD")
cascoda_make_binary(mtdapp)

if(CASCODA_OPENTHREAD_FTD)
	add_executable(ftdapp
		${PROJECT_SOURCE_DIR}/source/thread_dev_main.c
		${PROJECT_SOURCE_DIR}/source/thread_dev_api.c
		)
	target_include_directories(ftdapp
		PRIVATE ${PROJECT_SOURCE_DIR}/include
		)
	target_link_libraries(ftdapp ca821x-openthread-bm-ftd cascoda-bm-plat)
	target_compile_definitions(ftdapp PRIVATE APP_NAME="OT FTD")
	cascoda_make_binary(ftdapp)
endif()
