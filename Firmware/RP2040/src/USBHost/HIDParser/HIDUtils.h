#pragma once
#include <stdint.h>

class HIDUtils
{
public:
	HIDUtils() {}
	~HIDUtils() {}

	static uint32_t readBitsLE(uint8_t *buffer, uint32_t bitOffset, uint32_t bitLength);
};