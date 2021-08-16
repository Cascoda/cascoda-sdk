# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (ot-sed-sensorif)

if (NOT CASCODA_OPENTHREAD_FTD)
	message(WARNING "Skipping ot-sed-sensorif demo, constrained platform")
	return()
endif()

if (NOT TARGET ca821x-openthread-bm-mtd)
	message(WARNING "Skipping ot-sed-sensorif demo, openthread not built on this system")
	return()
endif()

if (NOT TARGET sensorif)
	message(WARNING "Skipping ot-sed-sensorif demo, sensorif not built on this system")
	return()
endif()

if (NOT OT_COAP)
	return()
endif()

add_executable(ot-sed-sensorif
	${PROJECT_SOURCE_DIR}/source/ot_sed_sensorif.c
	)

target_link_libraries(ot-sed-sensorif ca821x-openthread-bm-mtd sensorif tinycbor-master)
cascoda_make_binary(ot-sed-sensorif)
