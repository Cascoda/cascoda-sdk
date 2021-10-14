project(chilictl)

set(CASCODA_CHILICTL_TEST_SERIALNO "" CACHE STRING "Used for chilictl longtest: serialno of device to be flashed.")
set(CASCODA_CHILICTL_TEST_BINPATH "" CACHE STRING "Used for chilictl longtest: path to the binary to be flashed.")
mark_as_advanced(FORCE CASCODA_CHILICTL_TEST_SERIALNO)
mark_as_advanced(FORCE CASCODA_CHILICTL_TEST_BINPATH)

add_executable(chilictl
	${PROJECT_SOURCE_DIR}/common/Args.cpp
	${PROJECT_SOURCE_DIR}/common/DeviceInfo.cpp
	${PROJECT_SOURCE_DIR}/common/DeviceList.cpp
	${PROJECT_SOURCE_DIR}/external-flash/ExternalFlash.cpp
	${PROJECT_SOURCE_DIR}/external-flash/ExternalFlasher.cpp
	${PROJECT_SOURCE_DIR}/flash/Flash.cpp
	${PROJECT_SOURCE_DIR}/flash/Flasher.cpp
	${PROJECT_SOURCE_DIR}/list/List.cpp
	${PROJECT_SOURCE_DIR}/pipe/Pipe.cpp
	${PROJECT_SOURCE_DIR}/chilictl.cpp
	)

target_include_directories(chilictl
	PRIVATE
		${PROJECT_SOURCE_DIR}
)
	
target_link_libraries(chilictl
	ca821x-posix
)

cascoda_use_warnings(chilictl)
target_compile_options(chilictl PRIVATE -Wno-unused-parameter)

install(
	TARGETS
		chilictl
	COMPONENT
		tools
	RUNTIME DESTINATION
		${CMAKE_INSTALL_BINDIR}
)

if(NOT ("${CASCODA_CHILICTL_TEST_SERIALNO}" STREQUAL "") AND NOT ("${CASCODA_CHILICTL_TEST_BINPATH}" STREQUAL ""))
	cascoda_add_longtest(chilictl 20 flash -s ${CASCODA_CHILICTL_TEST_SERIALNO} -f ${CASCODA_CHILICTL_TEST_BINPATH})
	cascoda_add_longtest(chilictl 20 external -s ${CASCODA_CHILICTL_TEST_SERIALNO} -f ${CASCODA_CHILICTL_TEST_BINPATH})
endif()

