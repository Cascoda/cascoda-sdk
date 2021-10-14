project(ot-eink-server)

if (WIN32)
	return()
endif()

add_executable(ot-eink-server
	${PROJECT_SOURCE_DIR}/serverEink.c
	)

target_link_libraries(ot-eink-server ca821x-openthread-posix-ftd openthread-cli-ftd tinycbor-master)

install(
	TARGETS
		ot-eink-server
	COMPONENT
		examples
	RUNTIME DESTINATION
		${CMAKE_INSTALL_BINDIR}
)