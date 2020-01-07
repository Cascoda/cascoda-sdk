# Global config ---------------------------------------------------------------
project (nuvoton-vendor-lib)

add_library(nuvoton-vendor-nano120 OBJECT
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/adc.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/clk.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/crc.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/dac.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/ebi.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/fmc.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/gpio.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/i2c.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/i2s.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/lcd.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/pdma.c
#	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/retarget.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/rtc.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/sc.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/scuart.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/spi.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/sys.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/timer.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/uart.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/usbd.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/wdt.c
	${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/src/wwdt.c
	${PROJECT_SOURCE_DIR}/Nano100Library/Device/Nuvoton/Nano100Series/Source/system_Nano100Series.c
	${PROJECT_SOURCE_DIR}/Nano100Library/Device/Nuvoton/Nano100Series/Source/GCC/startup_Nano100Series.S
	)

target_include_directories(nuvoton-vendor-nano120
	PUBLIC
		${PROJECT_SOURCE_DIR}/Nano100Library/Device/Nuvoton/Nano100Series/Include
		${PROJECT_SOURCE_DIR}/Nano100Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/Nano100Library/CMSIS/Include
	)