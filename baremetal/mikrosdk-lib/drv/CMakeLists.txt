# Global config ---------------------------------------------------------------
project (mikrosdk-drv)

include(FetchContent)

set (MIKROSDK_DRV_REPO https://github.com/Cascoda/mikrosdk.git)

# version 2.5.1 the newest release in 2022 from MikroElektronika with our own modification code
set (MIKROSDK_DRV_TAG 003eb273673b3d4234eeacdb00f815a9f9dbe834) 

# Download the mikrosdk_drv_layer source
FetchContent_Declare(
    mikrosdk_drv_layer
    GIT_REPOSITORY ${MIKROSDK_DRV_REPO}
	GIT_TAG ${MIKROSDK_DRV_TAG}
)

FetchContent_GetProperties(mikrosdk_drv_layer)
if(NOT mikrosdk_drv_layer_POPULATED)
    FetchContent_Populate(mikrosdk_drv_layer)
endif()

add_library(mikrosdk-drv
    ${mikrosdk_drv_layer_SOURCE_DIR}/drv/lib/src/lib_drv_digital_in/drv_digital_in.c
    ${mikrosdk_drv_layer_SOURCE_DIR}/drv/lib/src/lib_drv_digital_out/drv_digital_out.c
    ${mikrosdk_drv_layer_SOURCE_DIR}/drv/lib/src/lib_drv_i2c_master/drv_i2c_master.c 
    ${mikrosdk_drv_layer_SOURCE_DIR}/drv/lib/src/lib_drv_spi_master/drv_spi_master.c
)

target_include_directories(mikrosdk-drv PUBLIC
    ${mikrosdk_drv_layer_SOURCE_DIR}/drv/lib/include
)

target_link_libraries(mikrosdk-drv PUBLIC
		sensorif mikrosdk-hal
)

target_compile_options(mikrosdk-drv PRIVATE -Wno-unused-variable -Wno-return-type)
