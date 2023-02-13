# Global config ---------------------------------------------------------------
project (ca821x-lwip-demo)

#Only build the lwip demo on the chili2
if(CASCODA_BUILD_LWIP AND TARGET chili2)
	add_executable(ot-cli-lwip
		${PROJECT_SOURCE_DIR}/source/thread_lwip_demo.c
		${PROJECT_SOURCE_DIR}/source/lwipdemo.c
		)
	target_include_directories(ot-cli-lwip
		PRIVATE ${PROJECT_SOURCE_DIR}/include
		)
	target_link_libraries(ot-cli-lwip openthread-cli-ftd ca821x-openthread-bm-ftd lwip-bm)
	cascoda_make_binary(ot-cli-lwip CASCODA_BUILD_BINARIES)
endif()
