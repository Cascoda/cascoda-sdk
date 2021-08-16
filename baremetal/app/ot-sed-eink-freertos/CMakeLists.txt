cmake_minimum_required (VERSION 3.12)
project (ot-sed-freertos)

if(NOT TARGET freertos)
	return()
endif()

if(NOT OT_COAP)
	return()
endif()

add_executable(ot-sed-eink-freertos
	${PROJECT_SOURCE_DIR}/source/sed_eink.c
)
target_include_directories(ot-sed-eink-freertos PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(ot-sed-eink-freertos
	ca821x-openthread-bm-mtd
	freertos
	uzlib
	sensorif
)

cascoda_configure_memory(ot-sed-eink-freertos 0x800 0x7000)
cascoda_make_binary(ot-sed-eink-freertos)
