project(serial-adapter)


add_executable(serial-adapter)

if(WIN32)
	target_sources(serial-adapter PRIVATE ${PROJECT_SOURCE_DIR}/serial-adapter-windows.c)
else()
	target_sources(serial-adapter PRIVATE ${PROJECT_SOURCE_DIR}/serial-adapter.c)
endif()

target_link_libraries(serial-adapter ca821x-posix)

install(
	TARGETS
		serial-adapter
	COMPONENT
		tools
	RUNTIME DESTINATION
		bin
)