#include <fstream>
#include <iostream>
#include <string.h>

constexpr size_t kSerialLen = 6;

struct knx_manufacturer_storage
{
	/// @brief used for checking whether storage has been written to - should be CA 5C OD A0
	uint8_t magic_number[4];
	/// @brief KNX Serial Number, as defined by the Point API specification
	uint8_t knx_serial_number[kSerialLen];
};

enum
{
	kFilename,
	kOutput,
	kSerial,
	kArgNum
};

int main(int argc, char** argv)
{
	if (argc != kArgNum)
	{
		std::cerr << "Usage: knx-gen-data [output-file.bin] [serial-number]\n";
		std::cerr << "\tCreate a binary file containing the KNX serial number\n";
		std::cerr << "\tready to be flashed onto the manufacturer page of a Chili.\n";
		return -1;
	}

	if (strlen(argv[kSerial]) != kSerialLen * 2)
	{
		std::cerr << "Serial number should be " << kSerialLen << "-byte hexadecimal string!\n";
		std::cerr << "\tExample: 00FA10010700\n";
		return -1;
	}

	struct knx_manufacturer_storage storage
	{
		{0xCA, 0x5C, 0x0D, 0xA0}, {}
	};

	for (int i = 0; i < kSerialLen; ++i)
	{
		std::string byte_str(argv[kSerial] + 2 * i, 2);
		uint8_t     byte             = strtol(byte_str.c_str(), NULL, 16);
		storage.knx_serial_number[i] = byte;
	}

	std::ofstream file(argv[kOutput], std::ios_base::binary);
	file.write((char*)&storage, sizeof(storage));

	return 0;
}