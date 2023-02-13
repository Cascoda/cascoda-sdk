# Global config ---------------------------------------------------------------
project (cascoda-sensorif)

# Main library config ---------------------------------------------------------
add_library(sensorif
	${PROJECT_SOURCE_DIR}/source/sif_si7021.c
	${PROJECT_SOURCE_DIR}/source/sif_max30205.c
	${PROJECT_SOURCE_DIR}/source/sif_ltr303als.c
	${PROJECT_SOURCE_DIR}/source/sif_il3820.c
	${PROJECT_SOURCE_DIR}/source/sif_il3820_image.c
	${PROJECT_SOURCE_DIR}/source/gfx_library.c
	${PROJECT_SOURCE_DIR}/source/gfx_driver.c
	${PROJECT_SOURCE_DIR}/source/sif_tmp102.c
	)

target_include_directories(sensorif
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif 
	PUBLIC
		cascoda-bm
		qr-code-generator
	)

add_library(eink-image
	${PROJECT_SOURCE_DIR}/source/sif_il3820_image.c
)

target_include_directories(eink-image
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)