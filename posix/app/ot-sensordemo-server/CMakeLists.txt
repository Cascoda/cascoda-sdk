project(ot-sensordemo-server)

if (WIN32)
	return()
endif()


add_executable(ot-sensordemo-server
	${PROJECT_SOURCE_DIR}/serverStandalone.c
	)
	
target_link_libraries(ot-sensordemo-server ca821x-openthread-posix-ftd openthread-cli-ftd tinycbor-master)

install(
	TARGETS
		ot-sensordemo-server
	COMPONENT
		examples
	RUNTIME DESTINATION
		bin
)