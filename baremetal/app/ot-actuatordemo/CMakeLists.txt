# Global config ---------------------------------------------------------------
project (ca821x-thread-actuatordemo)

#Only build the actuator demo on the chili2
if(TARGET cascoda-chili2)
	add_executable(cliapp-actuator-bm
		${PROJECT_SOURCE_DIR}/source/thread_actuator_bm.c
		${PROJECT_SOURCE_DIR}/source/actuatordemo.c
		)
	target_include_directories(cliapp-actuator-bm
		PRIVATE ${PROJECT_SOURCE_DIR}/include
		)
	target_link_libraries(cliapp-actuator-bm openthread-cli-ftd ca821x-openthread-bm-ftd cascoda-bm-plat tinycbor-master)
	cascoda_make_binary(cliapp-actuator-bm)
endif()
