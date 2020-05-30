project(posix-tests)

add_executable(evbme-get
	${PROJECT_SOURCE_DIR}/evbme-get.c
	)

add_executable(stress-test
	${PROJECT_SOURCE_DIR}/stress-test.c
	)

add_executable(security-test
	${PROJECT_SOURCE_DIR}/security-test.c
	)

add_executable(serial-test
	${PROJECT_SOURCE_DIR}/serial-test.c
	)

target_link_libraries(evbme-get ca821x-posix)
target_link_libraries(stress-test ca821x-posix)
target_link_libraries(security-test ca821x-posix)
target_link_libraries(serial-test ca821x-posix m)

cascoda_put_subdir(stress-test test)
cascoda_put_subdir(security-test test)
cascoda_put_subdir(serial-test test)

install(
	TARGETS
		evbme-get stress-test security-test serial-test
	COMPONENT
		tests
	RUNTIME DESTINATION
		${CMAKE_INSTALL_BINDIR}
)