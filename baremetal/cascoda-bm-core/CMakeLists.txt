# Global config ---------------------------------------------------------------
project (cascoda-bm-core)

# External flash (shares SPI Bus with CA-821x & uses PA15 as /CS)
if (CMAKE_SYSTEM_PROCESSOR STREQUAL M2351)
	if(NOT (CASCODA_CHILI2_CONFIG_STRING STREQUAL "ONE_SIDED"))
		set(CASCODA_EXTERNAL_FLASHCHIP_PRESENT ON CACHE BOOL "Enable internal logic for handling the external flash chip, sharing SPI bus with CA-821x and using PA15 as /CS.")
	else()
		set(CASCODA_EXTERNAL_FLASHCHIP_PRESENT OFF CACHE BOOL "Enable internal logic for handling the external flash chip, sharing SPI bus with CA-821x and using PA15 as /CS.")
	endif()
else()
	set(CASCODA_EXTERNAL_FLASHCHIP_PRESENT OFF CACHE BOOL "Enable internal logic for handling the external flash chip, sharing SPI bus with CA-821x and using PA15 as /CS.")
endif()

set(CASCODA_CORE_SPI ${CASCODA_SOURCE_DIR}/baremetal/cascoda-bm-core/source/cascoda_spi.c CACHE INTERNAL "")

set(CASCODA_SPI_BUF_TRACKING_ENABLED OFF CACHE BOOL "Enables the tracking of number of SPI buffers. IMPORTANT! As a side effect, this disables the _write() function's assert for if it's called within an interrupt!")

	add_library(cascoda-bm-core
		${PROJECT_SOURCE_DIR}/source/cascoda_log.c
		${PROJECT_SOURCE_DIR}/source/cascoda_spi.c
		${PROJECT_SOURCE_DIR}/source/cascoda_spi_secure.c
		${PROJECT_SOURCE_DIR}/source/cascoda_time.c
		${PROJECT_SOURCE_DIR}/source/cascoda_wait.c
	)

	if(CASCODA_CA_VER EQUAL 8212)
		target_sources(cascoda-bm-core PRIVATE ${PROJECT_SOURCE_DIR}/source/cascoda_dispatch_8212.c)
	else()
		target_sources(cascoda-bm-core PRIVATE ${PROJECT_SOURCE_DIR}/source/cascoda_dispatch.c)
	endif()

	target_link_libraries(cascoda-bm-core 
		PUBLIC
			cascoda-util
			cascoda-bm-plat
		)

	target_include_directories(cascoda-bm-core PUBLIC ${PROJECT_SOURCE_DIR}/include)

	target_compile_definitions(cascoda-bm-core PUBLIC 
		CASCODA_EXTERNAL_FLASHCHIP_PRESENT=$<BOOL:${CASCODA_EXTERNAL_FLASHCHIP_PRESENT}>
	)

    if (CASCODA_SPI_BUF_TRACKING_ENABLED)
        target_compile_definitions(cascoda-bm-core PUBLIC 
            CASCODA_SPI_BUF_TRACKING
        )
    endif()

	cascoda_use_warnings(cascoda-bm-core)
