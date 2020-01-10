project (freertos-demo)
cmake_minimum_required (VERSION 3.12)

if(${CASCODA_BM_USE_UART})
	return()
endif()

add_executable(freertos-nonsecure-demo
	${PROJECT_SOURCE_DIR}/source/main_ns.c
	${PROJECT_SOURCE_DIR}/source/mpu_demo.c
	${PROJECT_SOURCE_DIR}/source/tz_demo.c
	${PROJECT_SOURCE_DIR}/source/mpu_wrappers.c
)

target_include_directories(freertos-nonsecure-demo PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(freertos-nonsecure-demo PRIVATE freertos-secure-demo-implib freertos)

cascoda_make_binary(freertos-nonsecure-demo)
