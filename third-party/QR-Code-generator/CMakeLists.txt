project(QR-Code-generator)

add_library(qr-code-generator
    ${PROJECT_SOURCE_DIR}/c/qrcodegen.c
)

target_include_directories(qr-code-generator PUBLIC
    ${PROJECT_SOURCE_DIR}/c
)