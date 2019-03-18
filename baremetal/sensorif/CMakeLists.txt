# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (cascoda-sensorif)

# Main library config ---------------------------------------------------------
add_library(sensorif
	${PROJECT_SOURCE_DIR}/source/sif_si7021.c
	${PROJECT_SOURCE_DIR}/source/sif_max30205.c
	)

target_include_directories(sensorif
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif
	PUBLIC
		cascoda-bm
	)
