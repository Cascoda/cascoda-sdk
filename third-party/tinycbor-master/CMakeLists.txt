project(tinycbor-master)

add_library(tinycbor-master
    ${PROJECT_SOURCE_DIR}/src/cborerrorstrings.c
    ${PROJECT_SOURCE_DIR}/src/cborencoder.c
    ${PROJECT_SOURCE_DIR}/src/cborencoder_close_container_checked.c
    ${PROJECT_SOURCE_DIR}/src/cborparser.c
    ${PROJECT_SOURCE_DIR}/src/cborpretty.c
)

target_include_directories(tinycbor-master PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)