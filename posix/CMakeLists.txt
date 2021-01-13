add_subdirectory(app)
add_subdirectory(ca821x-posix)
add_subdirectory(test)

if(UNIX)
	add_subdirectory(ca821x-posix-thread)
endif()