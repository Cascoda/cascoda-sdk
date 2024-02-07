# Global config ---------------------------------------------------------------
project (cascoda-sensorif)

# Main library config ---------------------------------------------------------
add_library(sensorif
	${PROJECT_SOURCE_DIR}/source/sif_si7021.c
	${PROJECT_SOURCE_DIR}/source/sif_max30205.c
	${PROJECT_SOURCE_DIR}/source/sif_ltr303als.c
	${PROJECT_SOURCE_DIR}/source/sif_tmp102.c
	)

target_include_directories(sensorif
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif 
	PUBLIC
		cascoda-bm
	)
