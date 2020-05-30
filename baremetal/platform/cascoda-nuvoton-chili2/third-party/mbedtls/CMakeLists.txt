project (mbedtls-plat)

add_library(mbedcrypto-plat INTERFACE)

target_include_directories(mbedcrypto-plat
	INTERFACE
		${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(mbedcrypto-plat
	INTERFACE
		MBEDTLS_USER_CONFIG_FILE=\"mbedtls_device.h\"
)
