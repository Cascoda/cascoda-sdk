project(hidapi)

# Do not build for the Chilis, only for hosted targets
if(NOT WIN32 AND NOT UNIX)
    return()
endif()


if(WIN32)
    add_library(hidapi
        ${PROJECT_SOURCE_DIR}/windows/hid.c
    )

    target_include_directories(hidapi 
    PUBLIC
        ${PROJECT_SOURCE_DIR}/
    PRIVATE
        ${PROJECT_SOURCE_DIR}/hidapi/
    )

    target_link_libraries(hidapi PUBLIC setupapi)

elseif(UNIX)
    add_library(hidapi INTERFACE)

    target_include_directories(hidapi INTERFACE
        ${PROJECT_SOURCE_DIR}/
    )
endif()
