project (chili2-default-secure)

if(NOT CASCODA_CHILI2_TRUSTZONE)
	return()
endif()

add_executable(chili2-default-secure
	${PROJECT_SOURCE_DIR}/source/main_s.c
	$<TARGET_OBJECTS:cascoda-chili2-secure>
	$<TARGET_OBJECTS:nuvoton-vendor-m2351-secure>
)

target_include_directories(chili2-default-secure BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(chili2-default-secure
	freertos-secure
	cascoda-chili2-secure
	)

cascoda_make_binary(chili2-default-secure)

# Configure chili2-default-secure to build as secure, and produce an implib
cascoda_tz_secure(chili2-default-secure
	-T${PROJECT_SOURCE_DIR}/secure.ld
	-T${PROJECT_SOURCE_DIR}/nonsecure.ld
)