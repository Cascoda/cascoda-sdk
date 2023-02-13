# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-pwm-led-dimming)

# Main library config ---------------------------------------------------------
add_library(pwm-led
	${PROJECT_SOURCE_DIR}/source/pwm_led_dimming.c
	${PROJECT_SOURCE_DIR}/source/pwm_led_main.c
	)

target_include_directories(pwm-led
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

# link to the platform library that should be
# provided by the superproject and also the
# test15-4-api provided by the ca821x-api
target_link_libraries(pwm-led
	PUBLIC
		cascoda-bm
		test15-4-api
	)

# Adds TESTING_MODE to pwm-led
target_compile_definitions(pwm-led PRIVATE TESTING_MODE=$<BOOL:${TESTING_MODE}>)

# Test app config -------------------------------------------------------------
add_executable(pwm-led-dimming ${PROJECT_SOURCE_DIR}/source/pwm_led_main.c)
target_link_libraries(pwm-led-dimming pwm-led cascoda-bm)

# convert to bin file format
cascoda_make_binary(pwm-led-dimming CASCODA_BUILD_BINARIES)
