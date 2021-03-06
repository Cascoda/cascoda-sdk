# Global config ---------------------------------------------------------------
project (cascoda-bm-tests)

if(NOT BUILD_TESTING)
	return()
endif()

# Currently these tests only run on unix with the dummy bm plat
if(APPLE OR (NOT (UNIX OR MINGW)))
	return()
endif()

# Add tests -------------------------------------------------------------------
add_cmocka_test(time_test
	SOURCES
		${PROJECT_SOURCE_DIR}/time_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		cascoda-bm
		cascoda-bm-core
	LINK_OPTIONS
		-Wl,--wrap=BSP_Waiting
	)

add_cmocka_test(spi_test
	SOURCES
		${PROJECT_SOURCE_DIR}/spi_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		cascoda-bm
	LINK_OPTIONS
		-Wl,--wrap=BSP_Waiting,--wrap=BSP_SPIPopByte
	)

add_cmocka_test(wait_test
	SOURCES
		${PROJECT_SOURCE_DIR}/wait_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		cascoda-bm
		cascoda-bm-core
	LINK_OPTIONS
		-Wl,--wrap=BSP_Waiting,--wrap=BSP_SPIPopByte
	)

add_cmocka_test(dispatch_test
	SOURCES
		${PROJECT_SOURCE_DIR}/dispatch_test.c
	LINK_LIBRARIES
		${CMOCKA_SHARED_LIBRARY}
		cascoda-bm
		cascoda-bm-core
	LINK_OPTIONS
		-Wl,--wrap=BSP_Waiting,--wrap=SPI_Send
	)

cascoda_put_subdir(test
	time_test
	spi_test
	wait_test
	dispatch_test
)
