# Global config ---------------------------------------------------------------
cmake_minimum_required (VERSION 3.11)
project (cascoda-test15-4-bm)

# TxRx Config variables
set(CASCODA_TXRX_LED_TX "" CACHE STRING "Pin number to use for the tx LED in mac-dongle (or blank to use module default)")
set(CASCODA_TXRX_LED_RX "" CACHE STRING "Pin number to use for the rx LED in mac-dongle (or blank to use module default)")

# Main library config ---------------------------------------------------------
add_library(test15-4
	${PROJECT_SOURCE_DIR}/source/test15_4_evbme.c
	${PROJECT_SOURCE_DIR}/source/txrx_led.c
	)

target_include_directories(test15-4
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

# link to the platform library that should be
# provided by the superproject and also the
# test15-4-api provided by the ca821x-api
target_link_libraries(test15-4
	PUBLIC
		cascoda-bm
		test15-4-api
	)

# Add compile definitions for LED numbers if they are defined
target_compile_definitions(test15-4 PRIVATE
	$<$<BOOL:${CASCODA_TXRX_LED_TX}>:TXRX_LED_TX=${CASCODA_TXRX_LED_TX}>
	$<$<BOOL:${CASCODA_TXRX_LED_RX}>:TXRX_LED_RX=${CASCODA_TXRX_LED_RX}>
	)

# Test app config -------------------------------------------------------------
add_executable(mac-dongle ${PROJECT_SOURCE_DIR}/source/test15_4_main.c)
target_link_libraries(mac-dongle test15-4 cascoda-bm)

# convert to bin file format
cascoda_make_binary(mac-dongle)
