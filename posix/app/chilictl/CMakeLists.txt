project(chilictl)

add_executable(chilictl
	${PROJECT_SOURCE_DIR}/common/Args.cpp
	${PROJECT_SOURCE_DIR}/common/DeviceInfo.cpp
	${PROJECT_SOURCE_DIR}/common/DeviceList.cpp
	${PROJECT_SOURCE_DIR}/flash/Flash.cpp
	${PROJECT_SOURCE_DIR}/flash/Flasher.cpp
	${PROJECT_SOURCE_DIR}/list/List.cpp
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
