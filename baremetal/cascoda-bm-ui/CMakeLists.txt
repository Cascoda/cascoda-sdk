# Global config ---------------------------------------------------------------
project (cascoda-bm-ui)

# Main library config ---------------------------------------------------------
add_library(eink-driver-2-9
	${PROJECT_SOURCE_DIR}/source/sif_il3820.c
	${PROJECT_SOURCE_DIR}/source/sif_il3820_image.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-waveshare-1-54-full-res
	${PROJECT_SOURCE_DIR}/source/sif_ssd1681.c
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
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-mikroe-1-54-full-res-forGPIOexpander
	${PROJECT_SOURCE_DIR}/source/sif_ssd1608.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c 
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-mikroe-1-54-half-res
	${PROJECT_SOURCE_DIR}/source/sif_ssd1608.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c 
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(eink-driver-mikroe-1-54-half-res-forGPIOexpander
	${PROJECT_SOURCE_DIR}/source/sif_ssd1608.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c 
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
)
add_library(cascoda-btn
	${PROJECT_SOURCE_DIR}/source/cascoda_btn.c
)
add_library(btn-ext-pi4ioe5v6408
	${PROJECT_SOURCE_DIR}/source/cascoda_btn_ext.c
	${PROJECT_SOURCE_DIR}/source/sif_pi4ioe5v6408.c
	${PROJECT_SOURCE_DIR}/source/sif_btn_ext_pi4ioe5v6408.c
)
add_library(btn-ext-pi4ioe5v96248
	${PROJECT_SOURCE_DIR}/source/cascoda_btn_ext.c
	${PROJECT_SOURCE_DIR}/source/sif_pi4ioe5v96248.c
	${PROJECT_SOURCE_DIR}/source/sif_btn_ext_pi4ioe5v96248.c
)

target_compile_definitions(eink-driver-mikroe-1-54-full-res PUBLIC USE_BSP_SENSE=1)

target_compile_definitions(eink-driver-mikroe-1-54-half-res PUBLIC USE_BSP_SENSE=1)

target_compile_definitions(eink-driver-mikroe-1-54-full-res-forGPIOexpander PUBLIC USE_BSP_SENSE=0)

target_compile_definitions(eink-driver-mikroe-1-54-half-res-forGPIOexpander PUBLIC USE_BSP_SENSE=0)

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
target_include_directories(eink-driver-mikroe-1-54-full-res-forGPIOexpander
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(eink-driver-mikroe-1-54-half-res
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
)
target_include_directories(cascoda-btn
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(btn-ext-pi4ioe5v6408
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)
target_include_directories(btn-ext-pi4ioe5v96248
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_include_directories(eink-driver-mikroe-1-54-half-res-forGPIOexpander
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
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
target_link_libraries(eink-driver-mikroe-1-54-full-res-forGPIOexpander
	PUBLIC
		cascoda-bm
		qr-code-generator
		btn-ext-pi4ioe5v6408
	)
target_link_libraries(eink-driver-mikroe-1-54-half-res
	PUBLIC
		cascoda-bm
		qr-code-generator
	)
target_link_libraries(eink-driver-mikroe-1-54-half-res-forGPIOexpander
	PUBLIC
		cascoda-bm
		qr-code-generator
		btn-ext-pi4ioe5v6408
	)
target_link_libraries(cascoda-btn
	PUBLIC
		cascoda-bm
	)
target_link_libraries(btn-ext-pi4ioe5v6408
	PUBLIC
		cascoda-bm
		cascoda-btn
	)
target_link_libraries(btn-ext-pi4ioe5v96248
	PUBLIC
		cascoda-bm
		cascoda-btn
	)

target_compile_definitions(eink-driver-2-9 PUBLIC EPAPER_2_9_INCH)
target_compile_definitions(eink-driver-waveshare-1-54-half-res PUBLIC EPAPER_WAVESHARE_1_54_INCH)
target_compile_definitions(eink-driver-waveshare-1-54-full-res PUBLIC EPAPER_WAVESHARE_1_54_INCH EPAPER_FULL_RESOLUTION)
target_compile_definitions(eink-driver-mikroe-1-54-half-res PUBLIC EPAPER_MIKROE_1_54_INCH)
target_compile_definitions(eink-driver-mikroe-1-54-half-res-forGPIOexpander PUBLIC EPAPER_MIKROE_1_54_INCH)
target_compile_definitions(eink-driver-mikroe-1-54-full-res PUBLIC EPAPER_MIKROE_1_54_INCH EPAPER_FULL_RESOLUTION)
target_compile_definitions(eink-driver-mikroe-1-54-full-res-forGPIOexpander PUBLIC EPAPER_MIKROE_1_54_INCH EPAPER_FULL_RESOLUTION)

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
