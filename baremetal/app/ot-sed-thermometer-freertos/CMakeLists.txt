cmake_minimum_required (VERSION 3.12)
project (ot-sed-thermometer-freertos)

if(NOT TARGET freertos)
	return()
endif()

add_executable(ot-sed-thermometer-freertos
	${PROJECT_SOURCE_DIR}/source/sed_freertos.c
)
target_include_directories(ot-sed-thermometer-freertos PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(ot-sed-thermometer-freertos
	ca821x-openthread-bm-mtd
	freertos
	tinycbor-master
)

cascoda_configure_memory(ot-sed-thermometer-freertos 0x800 0x7000)
cascoda_make_binary(ot-sed-thermometer-freertos)
