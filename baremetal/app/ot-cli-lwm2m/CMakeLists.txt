# Global config ---------------------------------------------------------------
project (ot-cli-lwm2m)

#Only build on the chili2
if(TARGET cascoda-chili2)
	add_executable(ot-cli-lwm2m
		${PROJECT_SOURCE_DIR}/client/lwm2mclient.c
		${PROJECT_SOURCE_DIR}/client/object_access_control.c
		${PROJECT_SOURCE_DIR}/client/object_connectivity_moni.c
		${PROJECT_SOURCE_DIR}/client/object_connectivity_stat.c
		${PROJECT_SOURCE_DIR}/client/object_device.c
		${PROJECT_SOURCE_DIR}/client/object_firmware.c
		${PROJECT_SOURCE_DIR}/client/object_location.c
		${PROJECT_SOURCE_DIR}/client/object_security.c
		${PROJECT_SOURCE_DIR}/client/object_server.c
		${PROJECT_SOURCE_DIR}/client/object_test.c
		${PROJECT_SOURCE_DIR}/shared/platform.c
		)

	# Configure the client based on whether lwm2m security is enabled
	if(CASCODA_BUILD_SECURE_LWM2M)
		target_sources(ot-cli-lwm2m
			PRIVATE
				${PROJECT_SOURCE_DIR}/shared/mbedtlsconnection.c
			)
		target_compile_definitions(ot-cli-lwm2m PUBLIC WITH_MBEDTLS)
	else()
		target_sources(ot-cli-lwm2m
			PRIVATE
				${PROJECT_SOURCE_DIR}/shared/connection.c
			)
	endif()

	target_include_directories(ot-cli-lwm2m
		PRIVATE
			${PROJECT_SOURCE_DIR}/client
			${PROJECT_SOURCE_DIR}/shared
		)
	target_link_libraries(ot-cli-lwm2m openthread-cli-ftd ca821x-openthread-bm-ftd wakaama-client)
	cascoda_configure_memory(ot-cli-lwm2m 0x1000 0x9000)
	cascoda_make_binary(ot-cli-lwm2m)
endif()
