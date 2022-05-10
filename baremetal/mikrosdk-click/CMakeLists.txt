# Global config ---------------------------------------------------------------
project (mikrosdk-click)

# Main library config ---------------------------------------------------------
add_library(mikrosdk-click
	${PROJECT_SOURCE_DIR}/source/thermo.c
	${PROJECT_SOURCE_DIR}/source/thermo3.c
	${PROJECT_SOURCE_DIR}/source/motion.c
	${PROJECT_SOURCE_DIR}/source/relay.c
	${PROJECT_SOURCE_DIR}/source/environment2.c
	${PROJECT_SOURCE_DIR}/source/airquality4.c
	)

target_include_directories(mikrosdk-click
    PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(mikrosdk-click
	PUBLIC
		sensorif mikrosdk-drv mikrosdk-hal
	)