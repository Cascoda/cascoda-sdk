# Global config ---------------------------------------------------------------
# cmake 3.11 is required in order to use the neat external project syntax
cmake_minimum_required (VERSION 3.11)
project (nuvoton-vendor-lib)

enable_language(ASM)

if(FALSE)
add_library(nuvoton-vendor-m2351
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/acmp.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/bpwm.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/can.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/clk.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/crc.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/crypto.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/dac.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/eadc.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/ebi.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/ecap.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/epwm.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/fmc.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/gpio.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/i2c.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/i2s.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/pdma.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/qei.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/qspi.c
#	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/retarget.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/rtc.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/sc.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/scuart.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/sdh.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/spi.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/sys.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/timer.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/timer_pwm.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/uart.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usbd.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usci_i2c.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usci_spi.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usci_uart.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/wdt.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/wwdt.c
	${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Source/system_M2351.c
	${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Source/GCC/startup_M2351.S
	)

target_include_directories(nuvoton-vendor-m2351
	PUBLIC
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Include
		${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/M2351Library/CMSIS/Include
	)
endif()

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