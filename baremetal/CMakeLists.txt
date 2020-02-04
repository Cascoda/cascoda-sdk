# Global config ---------------------------------------------------------------
project (cascoda-baremetal)

# Baremetal platform
add_subdirectory(cascoda-bm-core)
add_subdirectory(cascoda-bm-driver)
add_subdirectory(sensorif)
add_subdirectory(platform/cascoda-nuvoton-chili)
add_subdirectory(platform/cascoda-nuvoton-chili2)
add_subdirectory(platform/cascoda-dummy-posix)

# Baremetal Applications (If a suitable platform layer was configured)
if(TARGET cascoda-bm-plat)
	# Openthread platform
	add_subdirectory(cascoda-bm-thread)
	# Examples
	add_subdirectory(app/eink-bm)
	add_subdirectory(app/ot-actuatordemo)
	add_subdirectory(app/ot-baremetal)
	add_subdirectory(app/ot-sed-eink)
	add_subdirectory(app/ot-sed-freertos)
	add_subdirectory(app/ot-sed-sensorif)
	add_subdirectory(app/ot-sed-standalone)
	add_subdirectory(app/sensorif-bm)
	add_subdirectory(app/tempsense-bm)
	add_subdirectory(app/test15_4-baremetal)
endif()
