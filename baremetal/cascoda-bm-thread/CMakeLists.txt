# Global config ---------------------------------------------------------------
project (ca821x-openthread)

# Main library config ---------------------------------------------------------
add_library(ca821x-openthread-bm-plat OBJECT
	${PROJECT_SOURCE_DIR}/platform/alarm.c
	${PROJECT_SOURCE_DIR}/platform/entropy.c
	${PROJECT_SOURCE_DIR}/platform/flash.c
	${PROJECT_SOURCE_DIR}/platform/logging.c
	${PROJECT_SOURCE_DIR}/platform/misc.c
	${PROJECT_SOURCE_DIR}/platform/radio.c
	${PROJECT_SOURCE_DIR}/platform/serial.c
	${PROJECT_SOURCE_DIR}/platform/settings.c
	)

target_link_libraries(ca821x-openthread-bm-plat
	PUBLIC
		cascoda-bm
	PRIVATE
		m
		openthread-plat-api
		mbedcrypto
	)

target_include_directories(ca821x-openthread-bm-plat
	PRIVATE
		${openthread_SOURCE_DIR}/src/core
		${PROJECT_SOURCE_DIR}/platform
	PUBLIC
		${openthread_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/include
	)

# Inject platform into openthread library to resolve cyclic dependency
add_library(ca821x-openthread-bm-ftd
	$<TARGET_OBJECTS:ca821x-openthread-bm-plat>
	$<TARGET_OBJECTS:openthread-ftd-obj>
)
target_link_libraries(ca821x-openthread-bm-ftd
	PUBLIC
		openthread-ftd-obj
		ca821x-openthread-bm-plat
	)

add_library(ca821x-openthread-bm-mtd
	$<TARGET_OBJECTS:ca821x-openthread-bm-plat>
	$<TARGET_OBJECTS:openthread-mtd-obj>
)
target_link_libraries(ca821x-openthread-bm-mtd
	PUBLIC
		openthread-mtd-obj
		ca821x-openthread-bm-plat
	)

# Test app config -------------------------------------------------------------
if(CASCODA_OPENTHREAD_FTD)
	add_executable(ot-cli
		${PROJECT_SOURCE_DIR}/example/thread_bm.c
		${PROJECT_SOURCE_DIR}/example/sensordemo.c
		)
	target_include_directories(ot-cli
		PRIVATE ${PROJECT_SOURCE_DIR}/platform
		)
	target_link_libraries(ot-cli openthread-cli-ftd ca821x-openthread-bm-ftd tinycbor-master)
	target_compile_definitions(ot-cli PRIVATE APP_NAME="OT CLI" OT_CLI=1)
	cascoda_make_binary(ot-cli)

	add_executable(ot-ncp
		${PROJECT_SOURCE_DIR}/example/thread_bm.c
		)
	target_include_directories(ot-ncp
		PRIVATE ${PROJECT_SOURCE_DIR}/platform
		)
	target_link_libraries(ot-ncp ca821x-openthread-bm-ftd openthread-ncp-ftd)
	target_compile_definitions(ot-ncp PRIVATE APP_NAME="OT NCP" OT_NCP=1)
	cascoda_make_binary(ot-ncp)
endif()
