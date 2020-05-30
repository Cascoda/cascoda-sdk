project(sniffer)

add_executable(sniffer
	${PROJECT_SOURCE_DIR}/sniffer.c
	)
	
target_link_libraries(sniffer ca821x-posix)

install(
	TARGETS
		sniffer
	COMPONENT
		tools
	RUNTIME DESTINATION
		${CMAKE_INSTALL_BINDIR}
)