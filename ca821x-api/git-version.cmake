cmake_minimum_required (VERSION 3.11)

# Find git
find_package(Git)

#Get the location of the source file to use as the git directory
get_filename_component(GIT_WORKDIR ${GITVER_IN} DIRECTORY)

if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --dirty --always --tags
		OUTPUT_VARIABLE CASCODA_GIT_VER
		OUTPUT_STRIP_TRAILING_WHITESPACE
		WORKING_DIRECTORY ${GIT_WORKDIR}
	)
endif()

if(NOT CASCODA_GIT_VER)
	set(CASCODA_GIT_VER "unknown")
endif()

message(STATUS "Cascoda SDK: ${CASCODA_GIT_VER}")

configure_file(
	"${GITVER_IN}"
	"${GITVER_OUT}"
	)
