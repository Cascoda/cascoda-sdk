cmake_minimum_required (VERSION 3.11)

# Create list of all source files
file(GLOB_RECURSE cascoda_allsource *.c *.cpp *.h *.hpp)
# Remove vendor code
foreach(TMP_PATH ${cascoda_allsource})
	if(${TMP_PATH} MATCHES "(/vendor/|third-party/)")
		list(REMOVE_ITEM cascoda_allsource ${TMP_PATH})
	endif()
endforeach()

# Find clang-format
find_program(
	CLANG_FORMAT_EXE
	NAMES "clang-format"
	DOC "Path to clang-format executable"
	)
if(NOT CLANG_FORMAT_EXE)
	message(FATAL_ERROR "clang-format not found.")
else()
	message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
endif()

execute_process(
	COMMAND ${CLANG_FORMAT_EXE} -version
	OUTPUT_VARIABLE CLANG_VERSION
	)

# version check
if(NOT ${CLANG_VERSION} MATCHES "version 6\.0\.1")
	message(FATAL_ERROR "clang-format must be version 6.0.1.")
endif()

# Run clang format
foreach(TMP_PATH ${cascoda_allsource})
	execute_process(COMMAND ${CLANG_FORMAT_EXE} -i ${TMP_PATH})
endforeach()
