project(posix-tests)

add_executable(evbme-get
	${PROJECT_SOURCE_DIR}/evbme-get.c
	)

add_executable(rand-test
	${PROJECT_SOURCE_DIR}/rand-test.c
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
target_link_libraries(rand-test ca821x-posix)
target_link_libraries(stress-test ca821x-posix)
target_link_libraries(security-test ca821x-posix)
target_link_libraries(serial-test ca821x-posix m)

cascoda_put_subdir(test
	rand-test
	stress-test
	security-test
	serial-test
)

install(
	TARGETS
		evbme-get rand-test stress-test security-test serial-test
	COMPONENT
		tests
	RUNTIME DESTINATION
		${CMAKE_INSTALL_BINDIR}
)