project(uzlib)

add_library(uzlib
    ${PROJECT_SOURCE_DIR}/src/adler32.c
    ${PROJECT_SOURCE_DIR}/src/crc32.c
    ${PROJECT_SOURCE_DIR}/src/defl_static.c
    ${PROJECT_SOURCE_DIR}/src/genlz77.c
    ${PROJECT_SOURCE_DIR}/src/tinfgzip.c
    ${PROJECT_SOURCE_DIR}/src/tinflate.c
    ${PROJECT_SOURCE_DIR}/src/tinfzlib.c
)

target_include_directories(uzlib PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)