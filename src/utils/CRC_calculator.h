#ifndef CRC_CALCULATOR_H_
#define CRC_CALCULATOR_H_
#include <stdint.h>
#include "config.h"

uint16_t crc_calculator(const Frame &frame);

#endif