add_library(iotivity-port STATIC IMPORTED GLOBAL)

if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/libiotivity-port.a OR NOT EXISTS ${CMAKE_BINARY_DIR}/lib/v0.22-2-Chili2-Lib.zip)
    file(DOWNLOAD
        https://github.com/Cascoda/cascoda-sdk/releases/download/v0.22-2/Chili2-Lib.zip
        ${CMAKE_BINARY_DIR}/lib/v0.22-2-Chili2-Lib.zip
    )

    file(ARCHIVE_EXTRACT
        INPUT ${CMAKE_BINARY_DIR}/lib/v0.22-2-Chili2-Lib.zip
        DESTINATION ${CMAKE_BINARY_DIR}/lib
    )
endif()

set_target_properties(iotivity-port PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libiotivity-port.a)

target_include_directories(iotivity-port INTERFACE
    ${PROJECT_SOURCE_DIR}/port/
    ${iotivitylite_SOURCE_DIR}/
    ${iotivitylite_SOURCE_DIR}/include
)