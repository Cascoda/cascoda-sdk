# EVBME Overview
The EVBME (Evaluation board management entity) is primarily responsible for linking the application, API and hardware drivers together. It contains various functions that the user must call to drive the rest of the system either from the context of low level platform-specific code or high level application-specific code.

| Low level function | Description |
|--------------------|-------------|
| \ref EVBMEHandler  | Must be called to read SPI messages from the ca821x device. |

| High level function | Description |
|---------------------|-------------|
|\ref EVBMEInitialise | Must be called at program initialisation to initialise the rest of the system. The ca821x_dev structure passed to this function must itself be initialised **before calling this function**. This means \ref ca821x_api_init and \ref ca821x_register_callbacks must both be called first. |
| \ref cascoda_io_handler | Must be called regularly by the application to process SPI/serial messages. |