# Global config ---------------------------------------------------------------
project (cascoda-sensorif)

# Main library config ---------------------------------------------------------
add_library(sensorif
	${PROJECT_SOURCE_DIR}/source/sif_si7021.c
	${PROJECT_SOURCE_DIR}/source/sif_max30205.c
	${PROJECT_SOURCE_DIR}/source/sif_ltr303als.c
	)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "M2351")
	target_sources(sensorif PRIVATE ${PROJECT_SOURCE_DIR}/source/sif_il3820.c)
endif()

target_include_directories(sensorif
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(sensorif 
	PUBLIC
		cascoda-bm
		qr-code-generator
	)