project (chili2-default-secure)

if(NOT CASCODA_CHILI2_TRUSTZONE)
	return()
endif()

add_executable(chili2-default-secure
	${PROJECT_SOURCE_DIR}/source/main_s.c
	$<TARGET_OBJECTS:chili2-secure>
	$<TARGET_OBJECTS:m2351-s>
)

target_include_directories(chili2-default-secure BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(chili2-default-secure
	freertos-secure
	chili2-secure
	)

cascoda_make_binary(chili2-default-secure CASCODA_BUILD_BINARIES)

# Configure chili2-default-secure to build as secure, and produce an implib
cascoda_tz_secure(chili2-default-secure
	-T${PROJECT_SOURCE_DIR}/secure.ld
	-T${PROJECT_SOURCE_DIR}/nonsecure.ld
)