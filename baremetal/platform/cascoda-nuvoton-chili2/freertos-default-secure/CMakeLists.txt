project (freertos-default-secure)
cmake_minimum_required (VERSION 3.12)

if(${CASCODA_BM_USE_UART})
	return()
endif()

add_executable(freertos-default-secure
	${PROJECT_SOURCE_DIR}/source/main_s.c
)

target_include_directories(freertos-default-secure BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(freertos-default-secure
	freertos-secure
	cascoda-chili2-secure
	nuvoton-vendor-m2351-secure
	)

cascoda_make_binary(freertos-default-secure)

# Configure freertos-default-secure to build as secure, and produce an implib
cascoda_tz_secure(freertos-default-secure
	-T${PROJECT_SOURCE_DIR}/secure.ld
	-T${PROJECT_SOURCE_DIR}/nonsecure.ld
)

# Set the implib to require the include directory as a transitive dependancy
target_include_directories(freertos-default-secure-implib BEFORE INTERFACE ${PROJECT_SOURCE_DIR}/include)
