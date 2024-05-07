# Chili2 Default Secure #

This TrustZone secure binary provides the secure portion of the Cascoda API, alongside the FreeRTOS secure API. It also handles booting into the non-secure world after initializing the hardware.

This target is used by flashing `chili2-default-secure` alongside your application. If you are using the Cascoda CMake build system, then the relevant objects will be automatically linked by setting the ``CASCODA_CHILI2_TRUSTZONE`` CMake cache variable. If not using the Cascoda CMake build system, you will have to make sure to link `CMSE_importLib.o` to your application, otherwise the application will not be able to access the secure functions provided. 

For further information on using this target, see the [TrustZone Development Guide](../../../../docs/dev/M2351-TrustZone-development-guide.md).