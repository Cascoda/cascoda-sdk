add_library(kis-port STATIC IMPORTED GLOBAL)

if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/libkis-port.a OR NOT EXISTS ${CMAKE_BINARY_DIR}/lib/v0.22-2-Chili2-Lib.zip)
    file(DOWNLOAD
        https://github.com/Cascoda/cascoda-sdk/releases/download/v0.22-2/Chili2-Lib.zip
        ${CMAKE_BINARY_DIR}/lib/v0.22-2-Chili2-Lib.zip
    )

    file(ARCHIVE_EXTRACT
        INPUT ${CMAKE_BINARY_DIR}/lib/v0.22-2-Chili2-Lib.zip
        DESTINATION ${CMAKE_BINARY_DIR}/lib
    )

    file(RENAME
        ${CMAKE_BINARY_DIR}/lib/Chili2-Lib/libiotivity-port.a
        ${CMAKE_BINARY_DIR}/lib/libiotivity-port.a
    )

    file(RENAME
        ${CMAKE_BINARY_DIR}/lib/Chili2-Lib/libkis-port.a
        ${CMAKE_BINARY_DIR}/lib/libkis-port.a
    )
endif()

set_target_properties(kis-port PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libkis-port.a)

if(OC_DEBUG_ENABLED)
    target_compile_definitions(kis-port INTERFACE OC_DEBUG)
endif()

if(OC_OSCORE_ENABLED)
   target_compile_definitions(kis-port INTERFACE OC_OSCORE)
   target_compile_definitions(kis-port INTERFACE OC_SPAKE)
endif()

target_include_directories(kis-port INTERFACE
    ${PROJECT_SOURCE_DIR}/port/
)

target_include_directories(mbedcrypto BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/port/knx-mbedtls-config)
target_include_directories(kisClientServer BEFORE PUBLIC ${PROJECT_SOURCE_DIR}/port/knx-mbedtls-config)

target_link_libraries(kis-port INTERFACE
    openthread-plat-api
    tinycbor-master
    ca821x-openthread-bm-plat
    cascoda-util
    mbedcrypto
)
