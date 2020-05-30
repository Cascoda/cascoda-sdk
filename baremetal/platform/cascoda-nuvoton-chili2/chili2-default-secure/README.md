# Chili2 Default Secure #

This TrustZone secure binary provides the secure portion of the Cascoda API, alongside the FreeRTOS secure API. It also handles booting into the non-secure world after initializing the hardware.

This target is used by flashing `chili2-default-secure` alongside your application. If not using the Cascoda CMake build system, you will have to make sure to link `CMSE_importLib.o` to your application, otherwise the application will not be able to access the secure functions provided.