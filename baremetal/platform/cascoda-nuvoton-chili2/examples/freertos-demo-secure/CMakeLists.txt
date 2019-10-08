project (freertos-demo-secure)
cmake_minimum_required (VERSION 3.12)

if(${CASCODA_BM_USE_UART})
	return()
endif()

get_target_property(DEFAULT_SECURE_SOURCE freertos-default-secure SOURCE_DIR)

add_executable(freertos-secure-demo
	${PROJECT_SOURCE_DIR}/source/main_s.c
	${PROJECT_SOURCE_DIR}/source/nsc_functions.c
)

target_include_directories(freertos-secure-demo BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(freertos-secure-demo freertos-secure nuvoton-vendor-m2351-secure)

cascoda_make_binary(freertos-secure-demo)

# Configure freertos-secure-demo to build as secure, and produce an implib
cascoda_tz_secure(freertos-secure-demo
	-T${DEFAULT_SECURE_SOURCE}/secure.ld
	-T${DEFAULT_SECURE_SOURCE}/nonsecure.ld
)

# Set the implib to require the include directory as a transitive dependancy
target_include_directories(freertos-secure-demo-implib BEFORE INTERFACE ${PROJECT_SOURCE_DIR}/include)