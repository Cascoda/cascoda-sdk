# API Hooks
This project hooks into the ca821x-api library in the following way:

| ca821x-api object                  | cascoda-bm-driver implementation |
|:-----------------------------------|:---------------------------------|
| ca821x_dev.ca821x_api_downstream   | SPI_Send                    |
| ca821x_wait_for_message            | SPI_Wait                    |

These objects are populated in the EVBMEInitialise function using the ca821x_dev parameter given by the application.

Note that the API callbacks must still be populated by the application.
