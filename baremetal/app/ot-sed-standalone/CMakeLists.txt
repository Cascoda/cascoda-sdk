cmake_minimum_required (VERSION 3.12)
project (ot-sed-standalone)

add_executable(sed-standalone
	${PROJECT_SOURCE_DIR}/source/sed_standalone.c
)

target_link_libraries(sed-standalone
	ca821x-openthread-bm-mtd
	cascoda-bm-plat
	tinycbor-master
)

cascoda_make_binary(sed-standalone)
