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

add_library(nuvoton-vendor-m2351-mkrom STATIC IMPORTED)
set_target_properties(nuvoton-vendor-m2351-mkrom PROPERTIES IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/src/libmkrom.a")

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

add_library(nuvoton-vendor-m2351-secure OBJECT
	${VENDOR_SOURCES}
	${VENDOR_SECURE_SOURCES}
)

target_include_directories(nuvoton-vendor-m2351-secure
	PUBLIC
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Include
		${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/M2351Library/CMSIS/Include
	)

cascoda_tz_build(nuvoton-vendor-m2351-secure)

add_library(nuvoton-vendor-m2351-nonsecure OBJECT
	${VENDOR_SOURCES}
)

target_include_directories(nuvoton-vendor-m2351-nonsecure
	PUBLIC
		${PROJECT_SOURCE_DIR}/M2351Library/Device/Nuvoton/M2351/Include
		${PROJECT_SOURCE_DIR}/M2351Library/StdDriver/inc
		${PROJECT_SOURCE_DIR}/M2351Library/CMSIS/Include
	)

target_link_libraries(nuvoton-vendor-m2351 PUBLIC nuvoton-vendor-m2351-mkrom)
target_link_libraries(nuvoton-vendor-m2351-secure PUBLIC nuvoton-vendor-m2351-mkrom)
target_link_libraries(nuvoton-vendor-m2351-nonsecure PUBLIC nuvoton-vendor-m2351-mkrom)

