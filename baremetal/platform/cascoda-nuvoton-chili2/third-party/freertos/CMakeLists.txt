project (freertos-lib)

add_library(freertos INTERFACE)

target_sources(freertos INTERFACE
	# Common source files
	${PROJECT_SOURCE_DIR}/source/croutine.c
	${PROJECT_SOURCE_DIR}/source/event_groups.c
	${PROJECT_SOURCE_DIR}/source/list.c
	${PROJECT_SOURCE_DIR}/source/queue.c
	${PROJECT_SOURCE_DIR}/source/stream_buffer.c
	${PROJECT_SOURCE_DIR}/source/tasks.c
	${PROJECT_SOURCE_DIR}/source/timers.c
	${PROJECT_SOURCE_DIR}/source/portable/heap_4.c
	#${PROJECT_SOURCE_DIR}/source/portable/mpu_wrappers.c
	# Platform-specific files
	${PROJECT_SOURCE_DIR}/source/portable-m2351/port.c
	${PROJECT_SOURCE_DIR}/source/portable-m2351/portasm.c
)

target_include_directories(freertos
	INTERFACE
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/include/portable-m2351
)

target_link_libraries(freertos
	INTERFACE
		nuvoton-vendor-m2351-nonsecure
)

add_library(freertos-secure INTERFACE)

target_sources(freertos-secure INTERFACE
	${PROJECT_SOURCE_DIR}/source/secure_context_port.c
	${PROJECT_SOURCE_DIR}/source/secure_context.c
	${PROJECT_SOURCE_DIR}/source/secure_init.c
	${PROJECT_SOURCE_DIR}/source/secure_heap.c
)

target_include_directories(freertos-secure
	INTERFACE
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/include/portable-m2351
)
