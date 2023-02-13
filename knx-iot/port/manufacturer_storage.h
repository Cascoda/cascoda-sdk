#ifndef MANUFACTURER_STORAGE_H
#define MANUFACTURER_STORAGE_H

#include <stdint.h>

struct knx_manufacturer_storage
{
	/// @brief used for checking whether storage has been written to - should be CA 5C OD A0
	uint8_t magic_number[4];
	/// @brief KNX Serial Number, as defined by the Point API specification
	uint8_t knx_serial_number[6];
};

/**
 * @brief Get the KNX serial number stored within the manufacturer storage flash page
 * 
 * @param output_buffer 6-byte array to copy binary serial number into
 * @return int 0 on success, 1 if the manufacturer storage is empty or corrupt
 */
int knx_get_stored_serial_number(uint8_t output_buffer[6]);

#endif // MANUFACTURER_STORAGE_H