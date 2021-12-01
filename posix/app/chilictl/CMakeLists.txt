project(chilictl)

set(CASCODA_CHILICTL_TEST_SERIALNO "" CACHE STRING "Used for chilictl longtest: serialno of device to be flashed.")
set(CASCODA_CHILICTL_TEST_OTA_SUPPORTED_APP_BINPATH "" CACHE STRING "Used for chilictl longtest: path to the application binary, with OTA support, to be flashed alongside the OTA bootloader.")
set(CASCODA_CHILICTL_TEST_OTA_NOT_SUPPORTED_APP_BINPATH "" CACHE STRING "Used for chilictl longtest: path to the application binary, without OTA support, to be flashed.")
set(CASCODA_CHILICTL_TEST_OTA_BOOTLOADER_BINPATH "" CACHE STRING "Used for chilictl longtest: path to the OTA bootloader binary to be flashed.")
mark_as_advanced(FORCE CASCODA_CHILICTL_TEST_SERIALNO) 
mark_as_advanced(FORCE CASCODA_CHILICTL_TEST_OTA_SUPPORTED_APP_BINPATH)
mark_as_advanced(FORCE CASCODA_CHILICTL_TEST_OTA_NOT_SUPPORTED_APP_BINPATH)
mark_as_advanced(FORCE CASCODA_CHILICTL_TEST_OTA_BOOTLOADER_BINPATH)

add_executable(chilictl
	${PROJECT_SOURCE_DIR}/common/Args.cpp
	${PROJECT_SOURCE_DIR}/common/DeviceInfo.cpp
	${PROJECT_SOURCE_DIR}/common/DeviceList.cpp
	${PROJECT_SOURCE_DIR}/flash/ExternalFlasher.cpp
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

if(NOT ("${CASCODA_CHILICTL_TEST_SERIALNO}" STREQUAL "") 
	AND NOT ("${CASCODA_CHILICTL_TEST_OTA_SUPPORTED_APP_BINPATH}" STREQUAL "")
	AND NOT ("${CASCODA_CHILICTL_TEST_OTA_NOT_SUPPORTED_APP_BINPATH}" STREQUAL "")
	AND NOT ("${CASCODA_CHILICTL_TEST_OTA_BOOTLOADER_BINPATH}" STREQUAL ""))
	cascoda_add_longtest(chilictl 30 flash -s ${CASCODA_CHILICTL_TEST_SERIALNO} -f ${CASCODA_CHILICTL_TEST_OTA_SUPPORTED_APP_BINPATH} -o ${CASCODA_CHILICTL_TEST_OTA_BOOTLOADER_BINPATH})
	cascoda_add_longtest(chilictl 20 flash -s ${CASCODA_CHILICTL_TEST_SERIALNO} -f ${CASCODA_CHILICTL_TEST_OTA_NOT_SUPPORTED_APP_BINPATH})
	cascoda_add_longtest(chilictl 20 flash -s ${CASCODA_CHILICTL_TEST_SERIALNO} -ef ${CASCODA_CHILICTL_TEST_OTA_NOT_SUPPORTED_APP_BINPATH})
endif()

