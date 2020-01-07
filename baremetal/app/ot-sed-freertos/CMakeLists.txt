cmake_minimum_required (VERSION 3.12)
project (ot-sed-freertos)

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
	tinycbor-master
)

cascoda_make_binary(sed-freertos)
