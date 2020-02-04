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

# Loop through all of the important variables and print them in a list
file(READ "CMakeCache.txt" CMAKE_CACHE)
macro(get_cachevar varname)
	string(REGEX MATCH "${varname}[^\n\r]*=[^\n\r]*" ${varname} "${CMAKE_CACHE}")
	string(REGEX MATCH "[^:=]+\$" ${varname} "${${varname}}")
endmacro()

get_cachevar(CASCODA_IMPORTANT_VARS)
foreach(ivar ${CASCODA_IMPORTANT_VARS})
	get_cachevar(${ivar})
	message(STATUS "${ivar}: ${${ivar}}")
endforeach(ivar)

configure_file(
	"${GITVER_IN}"
	"${GITVER_OUT}"
	)
