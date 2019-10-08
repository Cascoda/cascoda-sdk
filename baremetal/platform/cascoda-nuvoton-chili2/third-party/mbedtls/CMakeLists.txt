project (mbedtls-plat)
cmake_minimum_required (VERSION 3.12)

add_library(openthread-mbedtls-plat EXCLUDE_FROM_ALL
	${PROJECT_SOURCE_DIR}/TARGET_M2351/aes_alt.c
	${PROJECT_SOURCE_DIR}/TARGET_M2351/ecp_internal_alt.c
	${PROJECT_SOURCE_DIR}/TARGET_M2351/crypto-misc.cpp
)

target_include_directories(openthread-mbedtls-plat
	PRIVATE
		${PROJECT_SOURCE_DIR}/platform
		# We manually include the cascoda-chili2 interface includes here so we don't break trustzone builds
		# by linking to the incorrect version of the platform lib
		$<TARGET_PROPERTY:cascoda-chili2,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(openthread-mbedtls-plat
	PRIVATE
		openthread-mbedtls
		ca821x-api
)

target_compile_definitions(openthread-mbedtls-plat
	PUBLIC
		MBEDTLS_USER_CONFIG_FILE=\"mbedtls_device.h\"
	PRIVATE
		MBEDTLS_CONFIG_HW_SUPPORT
)
