# Script to run a list of tests with timeout. The 'LONGTEST_CSV_PATH' variable
# should be the path to a csv file containing lines in form:
# path_to_executable,timeout[,Additional_args]
cmake_minimum_required (VERSION 3.13.1)

# Macro to convert CSV line into a list and relevant variables
macro(extract_line line_var)
	# Split up the line, trailing entries are arguments
	string(REPLACE "," ";" LINELIST ${${line_var}})
	list(GET LINELIST 0 TESTPATH)
	list(GET LINELIST 1 TESTTIMEOUT)
	list(LENGTH LINELIST LINELISTLEN)
	set(ARGLIST "")
	if(LINELISTLEN GREATER_EQUAL "3")
		list(SUBLIST LINELIST 2 ${LINELISTLEN} ARGLIST)
	endif()
endmacro()

# Open the list of longtests
message("Discovering tests...")
file(STRINGS ${LONGTEST_CSV_PATH} LONGTEST_LIST)

# Search for '--gtest_list_tests' string, and expand test if found
set(EXPANDED_TEST_LIST "")
foreach(LINE IN LISTS LONGTEST_LIST)
	extract_line(LINE)
	if("--gtest_list_tests" IN_LIST LINELIST)
		# If the list_tests argument is included, use it to extract all of the tests and run them one by one
		execute_process(
			COMMAND ${TESTPATH} ${ARGLIST}
			TIMEOUT 5
			RESULT_VARIABLE STATUS
			OUTPUT_VARIABLE GTEST_NAMES
		)
		if(STATUS)
			message(FATAL_ERROR "Failed to extract google tests from ${TESTPATH}")
		endif()
		# Split output of gtest_list_tests into a list for easy processing
		string(REGEX REPLACE ";" "\\\\;" GTEST_NAMES "${GTEST_NAMES}")
		string(REGEX REPLACE "\n|\r\n" ";" GTEST_NAMES "${GTEST_NAMES}")
		# Parse the output for google test names
		set(GTEST_PREFIX "")
		foreach(GTEST IN LISTS GTEST_NAMES)
			# If its a test name, prepend the prefix and add to the test list. If not, set the prefix for the next tests.
			if(GTEST MATCHES "^  .*$")
				string(STRIP "${GTEST}" GTEST)
				# Create a comma seperated list of the other arguments
				set(CSARGLIST "")
				foreach(argument IN LISTS ARGLIST)
					if(NOT "--gtest_list_tests" STREQUAL argument)
						set(CSARGLIST "${ARGLIST},${argument}")
					endif()
				endforeach()
				list(APPEND EXPANDED_TEST_LIST "${TESTPATH},${TESTTIMEOUT},--gtest_filter=${GTEST_PREFIX}${GTEST}${CSARGLIST}")
			else()
				set(GTEST_PREFIX "${GTEST}")
			endif()
		endforeach()
	else()
		# If there is no special argument, just run the test as-is
		list(APPEND EXPANDED_TEST_LIST "${LINE}")
	endif()
endforeach()

# Log the number of tests
list(LENGTH EXPANDED_TEST_LIST EXPANDED_TEST_LIST_LEN)
message("${EXPANDED_TEST_LIST_LEN} tests discovered.")

#Summary string
set(SUMMARY_STRING "----------------------- TEST SUMMARY -----------------------\n")
macro(append_summary)
	foreach(ARG ${ARGN})
		set(SUMMARY_STRING "${SUMMARY_STRING} ${ARG}")
	endforeach()
endmacro()

foreach(LINE IN LISTS EXPANDED_TEST_LIST)
	extract_line(LINE)

	# Form the test name (for logging) from the executable name plus arguments.
	string(REPLACE ";" " " ARGSTRING "${ARGLIST}")
	get_filename_component(TESTNAME "${TESTPATH}" NAME)
	set(TESTNAME "${TESTNAME} ${ARGLIST}")

	# Run the test (up to 3 times)
	append_summary("- ${TESTNAME} [")
	foreach(i RANGE 1 7)
		message("Run ${i} of test ${TESTNAME}")
		append_summary("${i}:")
		execute_process(
			COMMAND ${TESTPATH} ${ARGLIST}
			TIMEOUT ${TESTTIMEOUT}
			RESULT_VARIABLE STATUS
		)
		# Check result
		if(NOT STATUS)
			message("TEST PASSED. Test: ${TESTNAME}.")
			append_summary("PASS")
			break()
		else()
			message("Test failed on run ${i} with error: ${STATUS}. Test: ${TESTNAME}.")
			append_summary("${STATUS},")
		endif()
	endforeach()

	append_summary("]\n")
	if(STATUS)
		message("${SUMMARY_STRING}")
		message(FATAL_ERROR "TEST FAILED, ERROR ${STATUS}. Test: ${TESTNAME}.")
	endif()
endforeach()

message("${SUMMARY_STRING}")
message("All long tests passed!")