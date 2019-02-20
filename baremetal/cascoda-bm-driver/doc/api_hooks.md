# API Hooks
This project hooks into the ca821x-api library in the following way:

| ca821x-api object                  | cascoda-bm-driver implementation |
|------------------------------------|----------------------------------|
| ca821x_dev.ca821x_api_downstream   | \ref SPI_Send                    |
| \ref ca821x_wait_for_message       | \ref SPI_Wait                    |

These objects are populated in the \ref EVBMEInitialise function using the \ref ca821x_dev parameter given by the application.

Note that the API callbacks must still be populated by the application.