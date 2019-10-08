cmake_minimum_required (VERSION 3.12)
project (ot-sed-freertos)

if (NOT CMAKE_HOST_UNIX)
	message(WARNING "Building Openthread on windows is a challenge. Contributions welcome but it's probably easier to switch to Linux.")
	message(WARNING "Skipping openthread build due to non-unix build system!")
	return()
endif()

if(NOT TARGET freertos-default-secure-implib)
	return()
endif()

add_executable(sed-freertos
	${PROJECT_SOURCE_DIR}/source/sed_freertos.c
)
target_include_directories(sed-freertos PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(sed-freertos PRIVATE BEFORE $<TARGET_PROPERTY:freertos-default-secure-implib,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(sed-freertos
	ca821x-openthread-bm-mtd
	freertos
	freertos-default-secure-implib
	cascoda-bm-plat-nonsecure
)

cascoda_make_binary(sed-freertos)
