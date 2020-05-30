cmake_minimum_required (VERSION 3.12)
project (ot-sed-thermometer)

add_executable(ot-sed-thermometer
	${PROJECT_SOURCE_DIR}/source/sed_standalone.c
)

target_link_libraries(ot-sed-thermometer
	ca821x-openthread-bm-mtd
	tinycbor-master
)

cascoda_make_binary(ot-sed-thermometer)
