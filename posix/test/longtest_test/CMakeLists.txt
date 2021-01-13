project(longtest_test)

add_executable(longtest_test
	${PROJECT_SOURCE_DIR}/longtest_test.c
	)

cascoda_put_subdir(test longtest_test)

cascoda_add_longtest(longtest_test 5 "Longtest" "is" "working.")
