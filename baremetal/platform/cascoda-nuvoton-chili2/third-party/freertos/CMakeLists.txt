project (freertos-lib)

add_library(freertos
	# Common source files
	${PROJECT_SOURCE_DIR}/source/croutine.c
	${PROJECT_SOURCE_DIR}/source/event_groups.c
	${PROJECT_SOURCE_DIR}/source/list.c
	${PROJECT_SOURCE_DIR}/source/queue.c
	${PROJECT_SOURCE_DIR}/source/stream_buffer.c
	${PROJECT_SOURCE_DIR}/source/tasks.c
	${PROJECT_SOURCE_DIR}/source/timers.c
	#${PROJECT_SOURCE_DIR}/source/portable/mpu_wrappers.c
	# Platform-specific files
	${PROJECT_SOURCE_DIR}/source/portable-m2351/cascoda_freertos_newlib.c
	${PROJECT_SOURCE_DIR}/source/portable-m2351/cascoda_os.c
	${PROJECT_SOURCE_DIR}/source/portable-m2351/cascoda_static.c
	${PROJECT_SOURCE_DIR}/source/portable-m2351/heap_cas.c
	${PROJECT_SOURCE_DIR}/source/portable-m2351/port.c
)

if(CASCODA_CHILI2_TRUSTZONE)
	target_sources(freertos PRIVATE ${PROJECT_SOURCE_DIR}/source/portable-m2351/portasm.c)
else()
	target_sources(freertos PRIVATE ${PROJECT_SOURCE_DIR}/source/portable-m2351/portasm_ntz.c)
endif()

target_include_directories(freertos
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/include/portable-m2351
)

target_link_libraries(freertos
	PRIVATE
		cascoda-bm
		nuvoton-vendor-m2351-nonsecure
)

if(CASCODA_CHILI2_TRUSTZONE)
	add_library(freertos-secure OBJECT
		${PROJECT_SOURCE_DIR}/source/secure_context_port.c
		${PROJECT_SOURCE_DIR}/source/secure_context.c
		${PROJECT_SOURCE_DIR}/source/secure_init.c
		${PROJECT_SOURCE_DIR}/source/secure_heap.c
	)

	target_include_directories(freertos-secure
		PUBLIC
			${PROJECT_SOURCE_DIR}/include
			${PROJECT_SOURCE_DIR}/include/portable-m2351
	)

	cascoda_tz_build(freertos-secure)

	target_compile_definitions(freertos-secure PUBLIC configENABLE_TRUSTZONE=1)
	target_compile_definitions(freertos PUBLIC configENABLE_TRUSTZONE=1)
else()
	target_compile_definitions(freertos PUBLIC configENABLE_TRUSTZONE=0)
endif()
