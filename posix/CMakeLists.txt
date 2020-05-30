add_subdirectory(app)
add_subdirectory(ca821x-posix)

if(UNIX)
	add_subdirectory(ca821x-posix-thread)
endif()