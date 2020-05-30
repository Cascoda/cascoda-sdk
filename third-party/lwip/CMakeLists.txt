project (cascoda-lwip)

if(CASCODA_BUILD_LWIP)
	set(LWIP_DIR ${PROJECT_SOURCE_DIR}/lwip-2.1.2)
	include(lwip-2.1.2/src/Filelists.cmake)

	add_library(lwip-bm
		${lwipnoapps_SRCS}
		${lwipallapps_SRCS}
		${PROJECT_SOURCE_DIR}/port/bm/lwip-port.c
		)

	target_include_directories(lwip-bm
		PUBLIC
			${PROJECT_SOURCE_DIR}/port/bm
			${LWIP_DIR}/src/include
		)

	target_link_libraries(lwip-bm
		PUBLIC
			mbedcrypto
			openthread-plat-api
		PRIVATE
			ca821x-api
			cascoda-util
		)

	if(TARGET freertos)
		add_library(lwip-freertos
			${lwipnoapps_SRCS}
			${lwipallapps_SRCS}
			${PROJECT_SOURCE_DIR}/port/freertos/lwip-port.c
			${PROJECT_SOURCE_DIR}/port/freertos/sys_arch.c
			)

		target_include_directories(lwip-freertos
			PUBLIC
				${PROJECT_SOURCE_DIR}/port/freertos
				${LWIP_DIR}/src/include
			)

		target_link_libraries(lwip-freertos
			PUBLIC
				mbedcrypto
				openthread-plat-api
				freertos
			PRIVATE
				ca821x-api
				cascoda-util
			)
	endif()
endif()