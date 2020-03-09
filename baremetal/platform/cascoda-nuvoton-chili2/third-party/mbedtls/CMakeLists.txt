project (mbedtls-plat)

add_library(mbedcrypto-plat EXCLUDE_FROM_ALL
	${PROJECT_SOURCE_DIR}/TARGET_M2351/aes_alt.c
	${PROJECT_SOURCE_DIR}/TARGET_M2351/hardware_poll.c
	${PROJECT_SOURCE_DIR}/TARGET_M2351/ecp_internal_alt.c
	${PROJECT_SOURCE_DIR}/TARGET_M2351/crypto-misc.cpp
)

target_include_directories(mbedcrypto-plat
	PRIVATE
		${PROJECT_SOURCE_DIR}/platform
		# We manually include the cascoda-chili2 interface includes here so we don't break TrustZone builds
		# by linking to the incorrect version of the platform library.
		$<TARGET_PROPERTY:cascoda-chili2,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(mbedcrypto-plat
	PRIVATE
		mbedcrypto
		ca821x-api
		nuvoton-vendor-m2351
)

target_compile_definitions(mbedcrypto-plat
	PUBLIC
		MBEDTLS_USER_CONFIG_FILE=\"mbedtls_device.h\"
	PRIVATE
		MBEDTLS_CONFIG_HW_SUPPORT
)
