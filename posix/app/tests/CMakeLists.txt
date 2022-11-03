project(posix-tests)

set(EXECUTABLE_NAMES
	evbme-get
	rand-test
	stress-test
	serial-test
)

# TODO: Implement a version of this test which is compatible with the CA8212
#       and then allow executable to be made unconditionally.
if(NOT (CASCODA_CA_VER EQUAL 8212))
	list(APPEND EXECUTABLE_NAMES security-test)
endif()

# Create the executable targets
foreach(exec_name ${EXECUTABLE_NAMES})
	add_executable(${exec_name}
		${PROJECT_SOURCE_DIR}/${exec_name}.c
		)
endforeach()

# Link exectuable targets with dependency
foreach(exec_name ${EXECUTABLE_NAMES})
	if(exec_name STREQUAL serial-test)
		target_link_libraries(${exec_name} ca821x-posix m)
	else()
		target_link_libraries(${exec_name} ca821x-posix)
	endif()
endforeach()

set(TEST_SUBDIR_TARGETS ${EXECUTABLE_NAMES})
list(REMOVE_ITEM TEST_SUBDIR_TARGETS evbme-get)
cascoda_put_subdir(test ${TEST_SUBDIR_TARGETS})

install(
	TARGETS
		${EXECUTABLE_NAMES}
	COMPONENT
		tests
	RUNTIME DESTINATION
		bin
)