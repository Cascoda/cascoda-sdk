cmake_minimum_required (VERSION 3.13)
project (ot-cli-lwip-freertos)

if(NOT TARGET freertos)
	return()
endif()

if(NOT CASCODA_BUILD_LWIP)
	return()
endif()

add_executable(ot-cli-lwip-freertos
	${PROJECT_SOURCE_DIR}/source/lwip_freertos_demo.c
	${PROJECT_SOURCE_DIR}/source/lwipcli.c
)
target_include_directories(ot-cli-lwip-freertos PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(ot-cli-lwip-freertos
	lwip-freertos
	openthread-cli-ftd
	ca821x-openthread-bm-ftd
)

cascoda_configure_memory(ot-cli-lwip-freertos 0x800 0x7000)
cascoda_make_binary(ot-cli-lwip-freertos CASCODA_BUILD_BINARIES)
