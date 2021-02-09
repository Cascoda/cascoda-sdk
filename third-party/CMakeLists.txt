
# UZLib, used for decompressing gzipped data
add_subdirectory(uzlib)

# qrcodegen, used for building QR codes
add_subdirectory(QR-Code-generator)

# hidapi, needed by the Windows build of ca821x-posix
add_subdirectory(hidapi)

# tinycbor-master, used for CBOR encoding and decoding
add_subdirectory(tinycbor-master)

# lwip, used for providing an expanded IP stack for Thread applications
add_subdirectory(lwip)

# wakaama, used for lwm2m stack
add_subdirectory(wakaama)
