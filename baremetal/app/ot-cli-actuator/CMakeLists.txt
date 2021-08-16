# Global config ---------------------------------------------------------------
project (ca821x-thread-actuatordemo)

if(NOT OT_COAP)
	return()
endif()

#Only build the actuator demo on the chili2
if(TARGET cascoda-chili2)
	add_executable(ot-cli-actuator
		${PROJECT_SOURCE_DIR}/source/thread_actuator_bm.c
		${PROJECT_SOURCE_DIR}/source/actuatordemo.c
		)
	target_include_directories(ot-cli-actuator
		PRIVATE ${PROJECT_SOURCE_DIR}/include
		)
	target_link_libraries(ot-cli-actuator openthread-cli-ftd ca821x-openthread-bm-ftd tinycbor-master)
	cascoda_make_binary(ot-cli-actuator)
endif()
