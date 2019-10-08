# FreeRTOS Default Secure #

This TrustZone secure binary provides the FreeRTOS secure API to a non-secure application, and boots into the non-secure world after initializing the hardware. Unlike `freertos-demo-secure`, it also provides the Cascoda baremetal drivers, which represent an abstraction that allows you to communicate to the CA821x on any supported platform.

This target is used by flashing `freertos-default-secure` alongside your application. You will have to make sure to link `CMSE_importLib.o` to your application, otherwise the application will not be able to access the secure functions provided here. See `freertos-demo` for an example of how to achieve this.

If you want to add additional secure functions, you must make a copy of this target and add the new API and sources. See `nsc_functions.h` and `nsc_functions.c` in freertos-demo-secure for an example of how this can be done.