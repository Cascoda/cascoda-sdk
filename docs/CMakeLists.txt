project (cascoda-docs)

configure_file(
	${PROJECT_SOURCE_DIR}/doxyfile.in
	${PROJECT_BINARY_DIR}/doxyfile
	)

# Find doxygen
find_package(Doxygen
        COMPONENTS dot
        OPTIONAL_COMPONENTS mscgen dia
        )

if(NOT TARGET Doxygen::doxygen)
	return()
endif()

# Add docs target
add_custom_target(docs
	COMMAND Doxygen::doxygen ${CMAKE_BINARY_DIR}/docs/doxyfile
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/etc/img ${PROJECT_BINARY_DIR}/docs/html/etc/img
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)

