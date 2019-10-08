cmake_minimum_required (VERSION 3.12)
project (ot-sed-standalone)

if (NOT CMAKE_HOST_UNIX)
	message(WARNING "Building Openthread on windows is a challenge. Contributions welcome but it's probably easier to switch to Linux.")
	message(WARNING "Skipping openthread build due to non-unix build system!")
	return()
endif()

add_executable(sed-standalone
	${PROJECT_SOURCE_DIR}/source/sed_standalone.c
)

target_link_libraries(sed-standalone
	ca821x-openthread-bm-mtd
	cascoda-bm-plat
)

cascoda_make_binary(sed-standalone)
