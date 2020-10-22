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
	add_subdirectory(app/ot-cli-actuator)
	add_subdirectory(app/ot-barebone)
	add_subdirectory(app/ot-cli-lwip)
	add_subdirectory(app/ot-cli-lwip-freertos)
	add_subdirectory(app/ot-sed-eink-freertos)
	add_subdirectory(app/ot-sed-thermometer-freertos)
	add_subdirectory(app/ot-sed-sensorif)
	add_subdirectory(app/ot-sed-thermometer)
	add_subdirectory(app/selftest-bm)
	add_subdirectory(app/sensorif-bm)
	add_subdirectory(app/mac-tempsense)
	add_subdirectory(app/mac-dongle)
	# Tests
	add_subdirectory(test)
endif()
