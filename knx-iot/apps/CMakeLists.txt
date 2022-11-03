project(knx-iot-apps)

# bodge: no mbedtls library exists for some reason? and some dependency expects
# it?  so we make one that just links in mbedcrypto
add_library(mbedtls INTERFACE) 
target_link_libraries(mbedtls INTERFACE mbedcrypto)

add_subdirectory(knx_iot_chilidev_sa)
add_subdirectory(knx_iot_chilidev_pb)
add_subdirectory(knx_iot_chilidev_therm)
