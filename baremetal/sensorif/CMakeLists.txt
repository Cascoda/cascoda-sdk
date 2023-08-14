# Global config ---------------------------------------------------------------
project (cascoda-sensorif)

# Main library config ---------------------------------------------------------
add_library(sensorif
	${PROJECT_SOURCE_DIR}/source/sif_si7021.c
	${PROJECT_SOURCE_DIR}/source/sif_max30205.c
	${PROJECT_SOURCE_DIR}/source/sif_ltr303als.c
	${PROJECT_SOURCE_DIR}/source/sif_tmp102.c
	)
add_library(eink-driver-2-9
	${PROJECT_SOURCE_DIR}/source/sif_il3820.c
	${PROJECT_SOURCE_DIR}/source/sif_il3820_image.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-waveshare-1-54-full-res
	${PROJECT_SOURCE_DIR}/source/sif_ssd1681.c
	${PROJECT_SOURCE_DIR}/source/sif_ssd16xx_image.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-waveshare-1-54-half-res
	${PROJECT_SOURCE_DIR}/source/sif_ssd1681.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-mikroe-1-54-full-res
	${PROJECT_SOURCE_DIR}/source/sif_ssd1608.c
	${PROJECT_SOURCE_DIR}/source/sif_ssd16xx_image.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-mikroe-1-54-half-res
	${PROJECT_SOURCE_DIR}/source/sif_ssd1608.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)

target_include_directories(sensorif
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-driver-2-9
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-driver-waveshare-1-54-full-res
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-driver-waveshare-1-54-half-res
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-driver-mikroe-1-54-full-res
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-driver-mikroe-1-54-half-res
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif 
	PUBLIC
		cascoda-bm
	)
target_link_libraries(eink-driver-2-9
	PUBLIC
		cascoda-bm
		qr-code-generator
	)
target_link_libraries(eink-driver-waveshare-1-54-full-res
	PUBLIC
		cascoda-bm
		qr-code-generator
	)
target_link_libraries(eink-driver-waveshare-1-54-half-res
	PUBLIC
		cascoda-bm
		qr-code-generator
	)
target_link_libraries(eink-driver-mikroe-1-54-full-res
	PUBLIC
		cascoda-bm
		qr-code-generator
	)
target_link_libraries(eink-driver-mikroe-1-54-half-res
	PUBLIC
		cascoda-bm
		qr-code-generator
	)

target_compile_definitions(eink-driver-2-9 PUBLIC EPAPER_2_9_INCH)
target_compile_definitions(eink-driver-waveshare-1-54-half-res PUBLIC EPAPER_WAVESHARE_1_54_INCH)
target_compile_definitions(eink-driver-waveshare-1-54-full-res PUBLIC EPAPER_WAVESHARE_1_54_INCH EPAPER_FULL_RESOLUTION)
target_compile_definitions(eink-driver-mikroe-1-54-half-res PUBLIC EPAPER_MIKROE_1_54_INCH)
target_compile_definitions(eink-driver-mikroe-1-54-full-res PUBLIC EPAPER_MIKROE_1_54_INCH EPAPER_FULL_RESOLUTION)

add_library(eink-image-2-9
	${PROJECT_SOURCE_DIR}/source/sif_il3820_image.c
)

add_library(eink-image-1-54
	${PROJECT_SOURCE_DIR}/source/sif_ssd16xx_image.c
)

target_include_directories(eink-image-2-9
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_include_directories(eink-image-1-54
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)