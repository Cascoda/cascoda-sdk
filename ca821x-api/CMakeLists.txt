# Global config ---------------------------------------------------------------
project (ca821x-api-lib)

# Set configuration variables -------------------------------------------------
cascoda_dropdown(CASCODA_CA_VER "CA-xxxx Version number of target part" 8211 8210 8212)

cascoda_dropdown(CASCODA_LOG_LEVEL
	"The minimum log level to print messages for (CRIT>WARN>NOTE>INFO>DEBG)"
	NOTE CRIT WARN INFO DEBG
	)
cascoda_mark_important(CASCODA_LOG_LEVEL)

set(CASCODA_MAC_BLACKLIST 0 CACHE STRING "The number of MAC-level blacklist entries. Setting this to 0 disables the blacklist feature.")
mark_as_advanced(CASCODA_MAC_BLACKLIST)

# Config file generation ------------------------------------------------------
configure_file(
	"${PROJECT_SOURCE_DIR}/include/ca821x_config.h.in"
	"${PROJECT_BINARY_DIR}/include/ca821x_config.h"
	)

# Add git-version target
set(GIT_VERSION_FILE ${PROJECT_BINARY_DIR}/source/ca821x_ver.c)
add_custom_target(git-version ALL
	BYPRODUCTS ${GIT_VERSION_FILE}
	COMMAND ${CMAKE_COMMAND}
		-DGITVER_IN="${PROJECT_SOURCE_DIR}/source/ca821x_ver.c.in"
		-DGITVER_OUT="${GIT_VERSION_FILE}"
		-P ${PROJECT_SOURCE_DIR}/git-version.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)

# Main library config ---------------------------------------------------------


add_library(ca821x-api
	${PROJECT_SOURCE_DIR}/source/ca821x_api.c
	${PROJECT_SOURCE_DIR}/source/ca821x_api_helper.c
	${PROJECT_SOURCE_DIR}/source/ca821x_error.c
	${GIT_VERSION_FILE}
)


add_dependencies(ca821x-api git-version)

target_include_directories(ca821x-api
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_BINARY_DIR}/include
	)
cascoda_use_warnings(ca821x-api)

# Link with the correct platform layer
if((UNIX OR MINGW) AND (NOT CASCODA_BUILD_DUMMY))
	target_link_libraries(ca821x-api PUBLIC ca821x-posix)
else()
	target_link_libraries(ca821x-api PUBLIC cascoda-bm)
endif()

# Test15-4 library ------------------------------------------------------------
add_library(test15-4-api
	${PROJECT_SOURCE_DIR}/test15_4/source/test15_4_phy_tests.c
	${PROJECT_SOURCE_DIR}/test15_4/source/test15_4_phy_tests_mac.c
	)


target_include_directories(test15-4-api
	PUBLIC
		${PROJECT_SOURCE_DIR}/test15_4/include
	)

target_link_libraries(test15-4-api
	PUBLIC
		ca821x-api
	)

cascoda_use_warnings(test15-4-api)

# Tests
add_subdirectory(test)
