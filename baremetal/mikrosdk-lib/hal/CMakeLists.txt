# Global config ---------------------------------------------------------------
project (mikrosdk-hal)

# Main library config ---------------------------------------------------------
add_library(mikrosdk-hal
	${PROJECT_SOURCE_DIR}/source/hal_gpio.c
	${PROJECT_SOURCE_DIR}/source/hal_i2c_master.c
	${PROJECT_SOURCE_DIR}/source/hal_spi_master.c
	${PROJECT_SOURCE_DIR}/source/hal_delay.c
	)

target_include_directories(mikrosdk-hal
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(mikrosdk-hal
	PUBLIC
		cascoda-bm
	)