# Global config ---------------------------------------------------------------
project (nuvoton-vendor-lib)

set(VENDOR_SOURCES
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/acmp.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/bpwm.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/can.c
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
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/timer.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/timer_pwm.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/uart.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usbd.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usci_i2c.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usci_spi.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/usci_uart.c
	${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Source/system_M2351.c
)

set(VENDOR_SECURE_SOURCES
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/clk.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/sys.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/wdt.c
	${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/wwdt.c
)

if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
	set(VENDOR_SOURCES
		${VENDOR_SOURCES}
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Source/GCC/startup_M2351.S
	)
elseif(CMAKE_C_COMPILER MATCHES armclang)
	set(VENDOR_SOURCES
		${VENDOR_SOURCES}
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Source/ARM/startup_M2351.s
	)
endif()

add_library(nuvoton-vendor-m2351 OBJECT
	${VENDOR_SOURCES}
	${VENDOR_SECURE_SOURCES}
)

target_include_directories(nuvoton-vendor-m2351
	PUBLIC
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Include
		${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/M2351Library/CMSIS/Include
	)

add_library(nuvoton-vendor-m2351-secure INTERFACE)

target_sources(nuvoton-vendor-m2351-secure INTERFACE
	${VENDOR_SOURCES}
	${VENDOR_SECURE_SOURCES}
)

target_include_directories(nuvoton-vendor-m2351-secure
	INTERFACE
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Include
		${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/M2351Library/CMSIS/Include
	)

target_compile_options(nuvoton-vendor-m2351-secure INTERFACE -mcmse)
target_link_options(nuvoton-vendor-m2351-secure INTERFACE -mcmse)

add_library(nuvoton-vendor-m2351-nonsecure INTERFACE)

target_sources(nuvoton-vendor-m2351-nonsecure INTERFACE
	${VENDOR_SOURCES}
)

target_include_directories(nuvoton-vendor-m2351-nonsecure
	INTERFACE
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Include
		${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/M2351Library/CMSIS/Include
	)
