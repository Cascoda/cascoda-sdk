project(cmocka-wrap-examples C)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMOCKA_PUBLIC_INCLUDE_DIRS}
)

add_executable(waiter_test_wrap waiter_test_wrap.c chef.c)
target_link_libraries(waiter_test_wrap ${CMOCKA_SHARED_LIBRARY})

add_test(NAME waiter_test_wrap COMMAND waiter_test_wrap)

set_target_properties(waiter_test_wrap
        PROPERTIES
        LINK_FLAGS  "-Wl,--wrap=chef_cook"
)
if (WIN32 OR MINGW OR CYGWIN)
    set_tests_properties(waiter_test_wrap PROPERTIES ENVIRONMENT "PATH=${DLL_PATH_ENV}")
endif (WIN32 OR MINGW OR CYGWIN)
